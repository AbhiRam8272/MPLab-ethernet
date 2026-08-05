/*
 * console.c - command interpreter for the standalone ORION controller
 *
 * Hardware independent: only console_putc/console_getc touch the UART, and
 * those live in console_uart.c. That keeps this parser unit-testable on a PC.
 *
 * Commands (case-insensitive, whitespace separated):
 *
 *   help                      list commands
 *   id                        read DEVICE_ID + REVISION
 *   probe                     re-run identification
 *   rd   <addr>               read one register           (addr in hex, 0..0x1FF)
 *   wr   <addr> <data>        write one register
 *   wrv  <addr> <data>        write then verify by read-back
 *   rmw  <addr> <lsb> <w> <v> read-modify-write a bit field
 *   dump [start] [count]      hex dump of a register range
 *   bcast <addr> <data>       broadcast write to all devices
 *   slv  [n]                  show / set the slave address (0x1F default)
 *
 *   tx | rx                   set the T/R switch
 *   pa   <0|1>                PA enable line
 *   load                      pulse the active stage-2 load strobe
 *   upd                       UPDATE_CODE 0->1->0 (register-driven load)
 *
 *   ph   <ch> <code>          set TX or RX phase for a channel (per T/R state)
 *   gn   <ch> <code>          set TX or RX gain  for a channel
 *   beam <idx>                write BEAM_CODE
 *   reset                     synchronous reset
 *
 *   lut  <start> <n>          stream n hex bytes into LUT space (prompted)
 */

#include <string.h>
#include "console.h"
#include "spi_hw.h"
#include "orion_hal.h"

/* Bump when commands change. Printed at boot and by 'ver', so a stale flash
 * is obvious without having to read the whole help text. */
#define FW_VERSION      "1.2"
#define FW_FEATURES     "lut,lutrd,epsilon"

/* Persistent antenna selection, used by 'ph'/'gn' and shown by 'ant'.
 * TX and RX are tracked separately because TR_MASK holds both nibbles. */
static uint8_t s_tx_ant = 0x0F;
static uint8_t s_rx_ant = 0x0F;

static orion_t *s_dev;
static char     s_line[CONSOLE_LINE_MAX];
static uint8_t  s_len;

/*
 * Bulk LUT loader state.
 *
 * Sending one 'wr' per byte costs ~14 characters of UART traffic for 1 byte of
 * payload. The 'lut' command switches the console into a raw hex-stream mode
 * so a 192-byte LUT arrives as a few compact lines instead of 192 commands.
 */
static bool     s_lut_active;
static uint16_t s_lut_addr;      /* next address to write */
static uint16_t s_lut_remaining; /* bytes still expected  */
static uint16_t s_lut_written;
static uint16_t s_lut_errors;
static uint8_t  s_lut_nib;       /* half-assembled byte   */
static bool     s_lut_have_nib;

/* --- tiny formatting helpers ---------------------------------------------- */
static const char HEXD[] = "0123456789ABCDEF";

void console_puts(const char *s)
{
    while (*s) {
        if (*s == '\n') console_putc('\r');
        console_putc(*s++);
    }
}

void console_print_hex8(uint8_t v)
{
    console_putc(HEXD[(v >> 4) & 0xF]);
    console_putc(HEXD[v & 0xF]);
}

void console_print_u16(uint16_t v)
{
    char buf[6];
    int8_t i = 0;
    if (v == 0) { console_putc('0'); return; }
    while (v && i < 5) { buf[i++] = (char)('0' + (v % 10u)); v /= 10u; }
    while (i--) console_putc(buf[i]);
}

static void print_hex16(uint16_t v)
{
    console_putc(HEXD[(v >> 12) & 0xF]);
    console_putc(HEXD[(v >>  8) & 0xF]);
    console_putc(HEXD[(v >>  4) & 0xF]);
    console_putc(HEXD[v & 0xF]);
}

/* --- parsing --------------------------------------------------------------- */
static char lower(char c) { return (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c; }

static bool streq_ci(const char *a, const char *b)
{
    while (*a && *b) { if (lower(*a) != lower(*b)) return false; a++; b++; }
    return *a == 0 && *b == 0;
}

/*
 * Parse an unsigned number. Accepts 0x-prefixed hex, or bare digits which are
 * treated as HEX (this is a register console - bare 1F must mean 0x1F).
 * Returns false on any invalid character.
 */
static bool parse_num(const char *s, uint16_t *out)
{
    uint32_t acc = 0;
    bool any = false;

    if (s == 0 || *s == 0) return false;
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) s += 2;

    while (*s) {
        char c = lower(*s++);
        uint8_t d;
        if      (c >= '0' && c <= '9') d = (uint8_t)(c - '0');
        else if (c >= 'a' && c <= 'f') d = (uint8_t)(c - 'a' + 10);
        else return false;
        acc = acc * 16u + d;
        if (acc > 0xFFFFu) return false;
        any = true;
    }
    if (!any) return false;
    *out = (uint16_t)acc;
    return true;
}

/* Split a line into argv, in place. Returns argc. */
static uint8_t tokenize(char *line, char *argv[], uint8_t maxargs)
{
    uint8_t argc = 0;
    char *p = line;

    while (*p && argc < maxargs) {
        while (*p == ' ' || *p == '\t') p++;
        if (*p == 0) break;
        argv[argc++] = p;
        while (*p && *p != ' ' && *p != '\t') p++;
        if (*p) *p++ = 0;
    }
    return argc;
}

static void print_mask(const char *label, uint8_t m, char pfx)
{
    uint8_t i;
    console_puts(label);
    console_puts(" = 0x");
    console_print_hex8((uint8_t)(m & 0x0Fu));
    console_puts("  (");
    if ((m & 0x0Fu) == 0u) {
        console_puts("none");
    } else {
        bool first = true;
        for (i = 0; i < 4u; i++) {
            if (m & (1u << i)) {
                if (!first) console_puts(", ");
                console_putc(pfx);
                console_putc('X');
                console_print_u16(i);
                first = false;
            }
        }
    }
    console_puts(")\n");
}

static void ok(void)    { console_puts("OK\n"); }
static void err(const char *m) { console_puts("ERR: "); console_puts(m); console_putc('\n'); }

static void report(orion_status_t st)
{
    switch (st) {
    case ORION_OK:          ok(); break;
    case ORION_ERR_ARG:     err("bad argument"); break;
    case ORION_ERR_VERIFY:  err("verify mismatch"); break;
    case ORION_ERR_ID:      err("device id mismatch"); break;
    case ORION_ERR_TIMEOUT: err("spi timeout"); break;
    default:                err("unknown"); break;
    }
}

/* --- commands -------------------------------------------------------------- */
void cmd_banner(void)
{
    console_puts("\n");
    console_puts("=========================================\n");
    console_puts(" ORION Beamformer Controller (dsPIC30F5011)\n");
    console_puts(" standalone SPI master - fw " FW_VERSION "\n");
    console_puts(" type 'help'\n");
    console_puts("=========================================\n");
}

static void cmd_help(void)
{
    console_puts(
        "\nRegisters:\n"
        "  id                     read DEVICE_ID + REVISION\n"
        "  probe                  re-identify device\n"
        "  rd  <addr>             read register (hex, 0..1FF)\n"
        "  wr  <addr> <data>      write register\n"
        "  wrv <addr> <data>      write + verify\n"
        "  rmw <addr> <lsb> <w> <v>  bit-field write\n"
        "  dump [start] [count]   hex dump\n"
        "  bcast <addr> <data>    broadcast write\n"
        "  slv [n]                show/set slave addr\n"
        "\nAntenna selection (bit0=CH0 .. bit3=CH3):\n"
        "  ant   [0-F]            show/set both TX and RX masks\n"
        "  txant [0-F]            1=TX0 2=TX1 4=TX2 8=TX3, F=all\n"
        "  rxant [0-F]            1=RX0 2=RX1 4=RX2 8=RX3, F=all\n"
        "\nBeam control:\n"
        "  tx | rx                set T/R switch\n"
        "  pa <0|1>               PA enable\n"
        "  ph <ch|all> <code>     phase code, 'all' = selected antennas\n"
        "  gn <ch|all> <code>     gain code,  'all' = selected antennas\n"
        "  beam <idx>             BEAM_CODE\n"
        "  load                   pulse stage-2 strobe\n"
        "  upd                    UPDATE_CODE pulse\n"
        "  reset                  sync reset\n"
        "\nEpsilon vendor tests:\n"
        "  etx  <ant> <p> <g> [max|nom|low] [av]  TX single point\n"
        "  erx  <ant> <p> <g> [dual|single]       RX single point\n"
        "  epa  <tx_mask> [stage]                 PA bias test\n"
        "  elna <rx_mask> [stage]                 LNA bias test\n"
        "  eadc [4-7]                             SAR ADC read\n"
        "  edet <0-3>                             detector RF measurement\n"
        "\nLUT bulk load:\n"
        "  lut   <start> <count>  then stream <count> bytes as raw hex\n"
        "  lutrd <start> <count>  read back a range as a hex block\n"
        "\n  ver                    firmware version\n"
        "\nNumbers are hex. LUT space is 100..1FF.\n");
}

static void cmd_id(void)
{
    uint8_t id = 0, rev = 0;
    if (orion_read(s_dev, ORION_REG_DEVICE_ID, &id) != ORION_OK) { err("spi"); return; }
    if (orion_read(s_dev, ORION_REG_REVISION,  &rev) != ORION_OK) { err("spi"); return; }

    console_puts("DEVICE_ID=0x"); console_print_hex8(id);
    console_puts(" REV=");        console_print_u16((uint16_t)(rev & 0x0F));
    console_putc('.');            console_print_u16((uint16_t)((rev >> 4) & 0x0F));
    if (id != ORION_DEVICE_ID_EXPECTED) console_puts("  <-- UNEXPECTED ID");
    console_putc('\n');
}

static void cmd_dump(uint16_t start, uint16_t count)
{
    uint16_t i;
    if (start > ORION_ADDR_MAX) { err("addr"); return; }
    if (count == 0 || count > 512u) count = 16u;
    if ((uint32_t)start + count > 512u) count = (uint16_t)(512u - start);

    for (i = 0; i < count; i++) {
        uint8_t v = 0;
        if ((i & 0x0Fu) == 0u) {
            if (i) console_putc('\n');
            print_hex16((uint16_t)(start + i));
            console_puts(": ");
        }
        if (orion_read(s_dev, (uint16_t)(start + i), &v) != ORION_OK) { err("spi"); return; }
        console_print_hex8(v);
        console_putc(' ');
    }
    console_putc('\n');
}

void cmd_init(orion_t *dev)
{
    s_dev = dev;
    s_len = 0;
}

void cmd_execute(char *line)
{
    char *argv[8];
    uint8_t argc = tokenize(line, argv, 8);
    uint16_t a, b, c, d;

    if (argc == 0) return;

    if (streq_ci(argv[0], "help") || streq_ci(argv[0], "?")) { cmd_help(); return; }

    if (streq_ci(argv[0], "ver")) {
        console_puts("firmware " FW_VERSION "  features: " FW_FEATURES "\n");
        return;
    }

    if (streq_ci(argv[0], "id")) { cmd_id(); return; }

    if (streq_ci(argv[0], "probe")) {
        orion_status_t st = orion_probe(s_dev);
        cmd_id();
        if (st == ORION_OK) console_puts("probe OK\n"); else report(st);
        return;
    }

    if (streq_ci(argv[0], "rd")) {
        uint8_t v;
        if (argc < 2 || !parse_num(argv[1], &a)) { err("usage: rd <addr>"); return; }
        if (a > ORION_ADDR_MAX) { err("addr > 1FF"); return; }
        if (orion_read(s_dev, a, &v) != ORION_OK) { err("spi"); return; }
        console_puts("["); print_hex16(a); console_puts("] = 0x");
        console_print_hex8(v); console_putc('\n');
        return;
    }

    if (streq_ci(argv[0], "wr")) {
        if (argc < 3 || !parse_num(argv[1], &a) || !parse_num(argv[2], &b)) {
            err("usage: wr <addr> <data>"); return;
        }
        if (a > ORION_ADDR_MAX || b > 0xFF) { err("range"); return; }
        report(orion_write(s_dev, a, (uint8_t)b));
        return;
    }

    if (streq_ci(argv[0], "wrv")) {
        if (argc < 3 || !parse_num(argv[1], &a) || !parse_num(argv[2], &b)) {
            err("usage: wrv <addr> <data>"); return;
        }
        if (a > ORION_ADDR_MAX || b > 0xFF) { err("range"); return; }
        report(orion_write_verify(s_dev, a, (uint8_t)b));
        return;
    }

    if (streq_ci(argv[0], "rmw")) {
        if (argc < 5 || !parse_num(argv[1], &a) || !parse_num(argv[2], &b)
                     || !parse_num(argv[3], &c) || !parse_num(argv[4], &d)) {
            err("usage: rmw <addr> <lsb> <width> <val>"); return;
        }
        report(orion_rmw(s_dev, a, (uint8_t)b, (uint8_t)c, (uint8_t)d));
        return;
    }

    if (streq_ci(argv[0], "dump")) {
        a = 0; b = 16;
        if (argc >= 2 && !parse_num(argv[1], &a)) { err("addr"); return; }
        if (argc >= 3 && !parse_num(argv[2], &b)) { err("count"); return; }
        cmd_dump(a, b);
        return;
    }

    if (streq_ci(argv[0], "bcast")) {
        if (argc < 3 || !parse_num(argv[1], &a) || !parse_num(argv[2], &b)) {
            err("usage: bcast <addr> <data>"); return;
        }
        report(orion_broadcast(a, (uint8_t)b));
        return;
    }

    if (streq_ci(argv[0], "slv")) {
        if (argc == 1) {
            console_puts("slave = 0x"); console_print_hex8(s_dev->slave); console_putc('\n');
        } else if (parse_num(argv[1], &a) && a <= ORION_SLV_MAX) {
            s_dev->slave = (uint8_t)a; ok();
        } else err("slave 0..3F");
        return;
    }

    /* --- control lines ---------------------------------------------------- */
    if (streq_ci(argv[0], "tx")) { orion_trx_set(ORION_MODE_TX); console_puts("T/R = TX\n"); return; }
    if (streq_ci(argv[0], "rx")) { orion_trx_set(ORION_MODE_RX); console_puts("T/R = RX\n"); return; }

    if (streq_ci(argv[0], "pa")) {
        if (argc < 2 || !parse_num(argv[1], &a)) { err("usage: pa <0|1>"); return; }
        orion_pa_enable(a != 0);
        console_puts(a ? "PA on\n" : "PA off\n");
        return;
    }

    if (streq_ci(argv[0], "load")) { orion_stg2_load(); ok(); return; }
    if (streq_ci(argv[0], "upd"))  { report(orion_update_code_pulse(s_dev)); return; }

    /* --- beam ------------------------------------------------------------- */
    if (streq_ci(argv[0], "ph")) {
        /* ph <ch|all> <code> - 'all' applies to the current antenna selection */
        uint8_t i, m;
        if (argc < 3 || !parse_num(argv[2], &b)) {
            err("usage: ph <ch|all> <code>"); return;
        }
        if (streq_ci(argv[1], "all")) {
            m = (orion_trx_get() == ORION_MODE_TX) ? s_tx_ant : s_rx_ant;
            if (m == 0u) { err("no antennas selected"); return; }
            for (i = 0; i < 4u; i++) {
                if (m & (1u << i)) {
                    orion_status_t st = (orion_trx_get() == ORION_MODE_TX)
                        ? orion_set_phase_tx(s_dev, i, (uint8_t)b)
                        : orion_set_phase_rx(s_dev, i, (uint8_t)b);
                    if (st != ORION_OK) { report(st); return; }
                }
            }
            ok();
            return;
        }
        if (!parse_num(argv[1], &a)) { err("usage: ph <ch|all> <code>"); return; }
        report(orion_trx_get() == ORION_MODE_TX
               ? orion_set_phase_tx(s_dev, (uint8_t)a, (uint8_t)b)
               : orion_set_phase_rx(s_dev, (uint8_t)a, (uint8_t)b));
        return;
    }

    if (streq_ci(argv[0], "gn")) {
        uint8_t i, m;
        if (argc < 3 || !parse_num(argv[2], &b)) {
            err("usage: gn <ch|all> <code>"); return;
        }
        if (streq_ci(argv[1], "all")) {
            m = (orion_trx_get() == ORION_MODE_TX) ? s_tx_ant : s_rx_ant;
            if (m == 0u) { err("no antennas selected"); return; }
            for (i = 0; i < 4u; i++) {
                if (m & (1u << i)) {
                    orion_status_t st = (orion_trx_get() == ORION_MODE_TX)
                        ? orion_set_gain_tx(s_dev, i, (uint8_t)b)
                        : orion_set_gain_rx(s_dev, i, (uint8_t)b);
                    if (st != ORION_OK) { report(st); return; }
                }
            }
            ok();
            return;
        }
        if (!parse_num(argv[1], &a)) { err("usage: gn <ch|all> <code>"); return; }
        report(orion_trx_get() == ORION_MODE_TX
               ? orion_set_gain_tx(s_dev, (uint8_t)a, (uint8_t)b)
               : orion_set_gain_rx(s_dev, (uint8_t)a, (uint8_t)b));
        return;
    }

    if (streq_ci(argv[0], "beam")) {
        if (argc < 2 || !parse_num(argv[1], &a)) { err("usage: beam <idx>"); return; }
        report(orion_set_beam(s_dev, (uint8_t)a));
        return;
    }

    if (streq_ci(argv[0], "reset")) { report(orion_sync_reset(s_dev)); return; }

    if (streq_ci(argv[0], "lut")) {
        if (argc < 3 || !parse_num(argv[1], &a) || !parse_num(argv[2], &b)) {
            err("usage: lut <start_addr> <count>");
            return;
        }
        if (a > ORION_ADDR_MAX)            { err("start > 1FF"); return; }
        if (b == 0)                        { err("count must be > 0"); return; }
        if ((uint32_t)a + b > (uint32_t)ORION_ADDR_MAX + 1u) {
            err("range overruns 0x1FF");
            return;
        }
        s_lut_active    = true;
        s_lut_addr      = a;
        s_lut_remaining = b;
        s_lut_written   = 0;
        s_lut_errors    = 0;
        s_lut_have_nib  = false;

        console_puts("send ");
        console_print_u16(b);
        console_puts(" bytes as hex (whitespace/newlines ok).\n");
        console_puts("'.' aborts.\n");
        return;
    }

    if (streq_ci(argv[0], "lutrd")) {
        /* Read back a LUT range as a compact hex block for host verification. */
        uint16_t i;
        if (argc < 3 || !parse_num(argv[1], &a) || !parse_num(argv[2], &b)) {
            err("usage: lutrd <start_addr> <count>");
            return;
        }
        if ((uint32_t)a + b > (uint32_t)ORION_ADDR_MAX + 1u) { err("range"); return; }
        for (i = 0; i < b; i++) {
            uint8_t v = 0;
            if (orion_read(s_dev, (uint16_t)(a + i), &v) != ORION_OK) { err("spi"); return; }
            console_print_hex8(v);
            if ((i & 0x1Fu) == 0x1Fu) console_putc('\n');
        }
        if ((b & 0x1Fu) != 0u) console_putc('\n');
        return;
    }

    /* ---- antenna / channel selection ------------------------------------ */
    if (streq_ci(argv[0], "ant")) {
        /* ant                -> show both masks
         * ant <mask>         -> set BOTH tx and rx
         */
        if (argc == 1) {
            print_mask("tx_ant", s_tx_ant, 'T');
            print_mask("rx_ant", s_rx_ant, 'R');
            return;
        }
        if (!parse_num(argv[1], &a) || a > 0x0Fu) {
            err("usage: ant [0-F]   (bit0=CH0 .. bit3=CH3)");
            return;
        }
        s_tx_ant = (uint8_t)a;
        s_rx_ant = (uint8_t)a;
        if (s_dev->present) {
            if (hal_set_tr_mask(s_dev, (int)a, (int)a) != ORION_OK) {
                err("spi"); return;
            }
        }
        print_mask("tx_ant", s_tx_ant, 'T');
        print_mask("rx_ant", s_rx_ant, 'R');
        return;
    }

    if (streq_ci(argv[0], "txant")) {
        if (argc == 1) { print_mask("tx_ant", s_tx_ant, 'T'); return; }
        if (!parse_num(argv[1], &a) || a > 0x0Fu) {
            err("usage: txant [0-F]   1=TX0 2=TX1 4=TX2 8=TX3, F=all");
            return;
        }
        s_tx_ant = (uint8_t)a;
        if (s_dev->present) {
            if (hal_set_tr_mask(s_dev, (int)a, -1) != ORION_OK) { err("spi"); return; }
        }
        print_mask("tx_ant", s_tx_ant, 'T');
        return;
    }

    if (streq_ci(argv[0], "rxant")) {
        if (argc == 1) { print_mask("rx_ant", s_rx_ant, 'R'); return; }
        if (!parse_num(argv[1], &a) || a > 0x0Fu) {
            err("usage: rxant [0-F]   1=RX0 2=RX1 4=RX2 8=RX3, F=all");
            return;
        }
        s_rx_ant = (uint8_t)a;
        if (s_dev->present) {
            if (hal_set_tr_mask(s_dev, -1, (int)a) != ORION_OK) { err("spi"); return; }
        }
        print_mask("rx_ant", s_rx_ant, 'R');
        return;
    }

    /* ---- epsilon vendor test sequences ---------------------------------- */
    if (streq_ci(argv[0], "etx")) {
        /* etx <ant_sel> <p_idx> <g_idx> [bias] [final_av] */
        hal_bias_t bias = HAL_BIAS_MAX;
        uint16_t av = 31;
        if (argc < 4) {
            err("usage: etx <ant|.> <p_idx> <g_idx> [max|nom|low] [final_av]");
            return;
        }
        /* '.' means "use the mask set by txant". Checked before parse_num,
         * which would reject a non-hex token. */
        if (argv[1][0] == '.') a = s_tx_ant;
        else if (!parse_num(argv[1], &a)) {
            err("usage: etx <ant|.> <p_idx> <g_idx> [max|nom|low] [final_av]");
            return;
        }
        if (!parse_num(argv[2], &b) || !parse_num(argv[3], &c)) {
            err("usage: etx <ant|.> <p_idx> <g_idx> [max|nom|low] [final_av]");
            return;
        }
        if (argc >= 5) {
            if      (streq_ci(argv[4], "max")) bias = HAL_BIAS_MAX;
            else if (streq_ci(argv[4], "nom")) bias = HAL_BIAS_NOM;
            else if (streq_ci(argv[4], "low")) bias = HAL_BIAS_LOW;
            else { err("bias must be max|nom|low"); return; }
        }
        if (argc >= 6 && !parse_num(argv[5], &av)) { err("final_av"); return; }
        console_puts("epsilon TX single point\n");
        report(hal_tx_single_point(s_dev, (uint8_t)a, (uint8_t)b,
                                   (uint8_t)c, bias, (uint8_t)av));
        return;
    }

    if (streq_ci(argv[0], "erx")) {
        /* erx <ant_sel> <p_idx> <g_idx> [dual|single] */
        bool dual = true;
        if (argc < 4) {
            err("usage: erx <ant|.> <p_idx> <g_idx> [dual|single]");
            return;
        }
        if (argv[1][0] == '.') a = s_rx_ant;
        else if (!parse_num(argv[1], &a)) {
            err("usage: erx <ant|.> <p_idx> <g_idx> [dual|single]");
            return;
        }
        if (!parse_num(argv[2], &b) || !parse_num(argv[3], &c)) {
            err("usage: erx <ant|.> <p_idx> <g_idx> [dual|single]");
            return;
        }
        if (argc >= 5) dual = streq_ci(argv[4], "dual");
        console_puts("epsilon RX single point\n");
        report(hal_rx_single_point(s_dev, (uint8_t)a, (uint8_t)b,
                                   (uint8_t)c, dual));
        return;
    }

    if (streq_ci(argv[0], "epa")) {
        /* epa <tx_mask> [0|1]  - stage 0 arms, stage 1 toggles TR */
        uint16_t stage = 0;
        if (argc < 2 || !parse_num(argv[1], &a)) {
            err("usage: epa <tx_mask> [stage]"); return;
        }
        if (argc >= 3 && !parse_num(argv[2], &stage)) { err("stage"); return; }
        report(hal_pa_bias_test(s_dev, (uint8_t)a, 127, 0, (int)stage));
        if (stage == 0) console_puts("PA bias armed - measure, then 'epa <mask> 1'\n");
        return;
    }

    if (streq_ci(argv[0], "elna")) {
        uint16_t stage = 0;
        if (argc < 2 || !parse_num(argv[1], &a)) {
            err("usage: elna <rx_mask> [stage]"); return;
        }
        if (argc >= 3 && !parse_num(argv[2], &stage)) { err("stage"); return; }
        report(hal_lna_bias_test(s_dev, (uint8_t)a, 0, 127, (int)stage));
        if (stage == 0) console_puts("LNA bias armed - measure, then 'elna <mask> 1'\n");
        return;
    }

    if (streq_ci(argv[0], "eadc")) {
        uint16_t gp = 7, val = 0;
        if (argc >= 2 && !parse_num(argv[1], &gp)) { err("usage: eadc [4-7]"); return; }
        if (hal_sar_adc_read(s_dev, (uint8_t)gp, &val) != ORION_OK) {
            err("adc read"); return;
        }
        console_puts("SAR ADC OUTPUT: ");
        console_print_u16(val);
        console_puts("  (0-511)\n");
        return;
    }

    if (streq_ci(argv[0], "edet")) {
        int16_t rf = 0; uint8_t fl[4]; uint8_t i;
        if (argc < 2 || !parse_num(argv[1], &a) || a > 3u) {
            err("usage: edet <0-3>"); return;
        }
        if (hal_det_measure_rf_only(s_dev, (uint8_t)a, &rf, fl) != ORION_OK) {
            err("detector"); return;
        }
        console_puts("SAR ADC (RF component): ");
        if (rf < 0) { console_putc('-'); rf = (int16_t)(-rf); }
        console_print_u16((uint16_t)rf);
        console_putc('\n');
        for (i = 0; i < 4u; i++) {
            console_puts("Flash ADC DET");
            console_print_u16(i);
            console_puts(": ");
            console_print_u16(fl[i]);
            console_putc('\n');
        }
        return;
    }

    err("unknown command - try 'help'");
}

/* --- line editor ----------------------------------------------------------- */
/*
 * Consume one character of the raw hex stream. Returns true while the loader
 * is still active, so the caller knows not to treat it as command input.
 *
 * Shared by every transport: the UART poll loop and the TCP command server
 * both route received bytes through here first.
 */
bool cmd_lut_active(void) { return s_lut_active; }

bool cmd_lut_feed(char ch)
{
    uint8_t d;

    if (!s_lut_active) return false;

    if (ch == '.') {                       /* abort */
        s_lut_active = false;
        console_puts("\nLUT aborted after ");
        console_print_u16(s_lut_written);
        console_puts(" bytes\n");
        return true;
    }

    if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n' || ch == ',')
        return true;                       /* separators are free */

    if      (ch >= '0' && ch <= '9') d = (uint8_t)(ch - '0');
    else if (ch >= 'a' && ch <= 'f') d = (uint8_t)(ch - 'a' + 10);
    else if (ch >= 'A' && ch <= 'F') d = (uint8_t)(ch - 'A' + 10);
    else return true;                      /* ignore junk */

    if (!s_lut_have_nib) {
        s_lut_nib      = d;
        s_lut_have_nib = true;
        return true;
    }

    /* Second nibble completes a byte - write it straight through. */
    {
        uint8_t byte = (uint8_t)((s_lut_nib << 4) | d);
        s_lut_have_nib = false;

        if (orion_write(s_dev, s_lut_addr, byte) != ORION_OK) s_lut_errors++;
        s_lut_addr++;
        s_lut_written++;
        s_lut_remaining--;

        /* Progress tick every 32 bytes so the host can pace itself. */
        if ((s_lut_written & 0x1Fu) == 0u) console_putc('#');

        if (s_lut_remaining == 0u) {
            s_lut_active = false;
            console_puts("\nLUT done: ");
            console_print_u16(s_lut_written);
            console_puts(" bytes, ");
            console_print_u16(s_lut_errors);
            console_puts(" errors\n");
            console_puts("orion> ");
        }
    }
    return true;
}

void console_poll(void)
{
    char ch;

    while (console_getc(&ch)) {
        if (cmd_lut_feed(ch)) continue;
        if (ch == '\r' || ch == '\n') {
            console_putc('\r'); console_putc('\n');
            s_line[s_len] = 0;
            if (s_len) cmd_execute(s_line);
            s_len = 0;
            console_puts("orion> ");
        } else if (ch == 0x08 || ch == 0x7F) {      /* backspace / delete */
            if (s_len) {
                s_len--;
                console_puts("\b \b");
            }
        } else if (ch >= 0x20 && ch < 0x7F) {
            if (s_len < CONSOLE_LINE_MAX - 1u) {
                s_line[s_len++] = ch;
                console_putc(ch);                    /* echo */
            }
        }
    }
}
