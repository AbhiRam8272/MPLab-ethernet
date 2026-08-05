/*
 * orion_spi_udig.c - spi_hw.h implemented as a bit-bang master on the
 *                    UDIGPSB0183 board (PIC32MX795F512L, 80 MHz)
 *
 * Why bit-bang and not the SPI peripheral:
 *
 *   Your app already occupies every hardware-SPI pin group.
 *     SPI1  RD0 / RD9 / RD10  -> PS3_5, PS4_1, PS3_6 (and the LMX bit-bang
 *                                macros SCK/LE_1/SDO in app.h)
 *     SPI2  RG6..RG9          -> Ethernet RMII + PS7_5 / PS7_4
 *
 *   Rather than take pins away from your phase shifters, ORION runs on six
 *   genuinely free pins. For a 3-byte frame this costs nothing measurable,
 *   and it makes CS framing exact: the ORION protocol requires CS to stay
 *   LOW across all three bytes, which hardware SS on a PIC32 will not do.
 *
 * Mode 0 (CPOL = 0, CPHA = 0):
 *   - SCK idles LOW
 *   - MOSI is set while SCK is LOW
 *   - the slave samples on the RISING edge
 *   - we sample MISO just before driving SCK low again
 *   - MSB first
 */

#include <stdint.h>
#include <stdbool.h>

#include "definitions.h"
#include "spi_hw.h"
#include "orion_board.h"

/* --------------------------------------------------------------------------
 * Delay
 * --------------------------------------------------------------------------
 * CP0 Count ticks at SYSCLK/2 = 40 MHz, i.e. 40 ticks per microsecond.
 * Confirmed against your initialization.c: FNOSC=PRIPLL, POSCMOD=EC,
 * FPLLIDIV=DIV_2, FPLLMUL=MUL_20, FPLLODIV=DIV_1  ->  SYSCLK 80 MHz.
 */
#define CP0_TICKS_PER_US    (CPU_CLOCK_FREQUENCY / 2000000UL)

void spi_hw_delay_us(uint16_t us)
{
    uint32_t start = _CP0_GET_COUNT();
    uint32_t want  = (uint32_t) us * CP0_TICKS_PER_US;

    /* Unsigned subtraction, so this is correct across the 32-bit wrap. */
    while ((_CP0_GET_COUNT() - start) < want)
    {
        /* spin */
    }
}

/* Half-bit delay. At ORION_SPI_HALF_US == 0 this collapses to nothing and the
 * clock runs as fast as the GPIO writes allow (a few MHz). */
static inline void half_bit(void)
{
#if ORION_SPI_HALF_US > 0
    spi_hw_delay_us(ORION_SPI_HALF_US);
#endif
}

/* --------------------------------------------------------------------------
 * Init
 * -------------------------------------------------------------------------- */
void spi_hw_init(void)
{
    /*
     * Directions come from MHC (Pin Settings), so this only establishes the
     * idle line state. Getting this wrong is the classic cause of a phantom
     * first byte.
     */
    ORION_PIN_CS_Set();                 /* nCS idle HIGH  = deselected */
//    ORION_PIN_SCK_Clear();              /* SCK idle LOW   = mode 0     */
//    ORION_PIN_SDO_Clear();

#if ORION_HAS_TR
    ORION_PIN_TR_Clear();               /* safe idle: RX */
#endif
#if ORION_HAS_PA
    ORION_PIN_PA_Clear();               /* PA off */
#endif

    spi_hw_delay_us(10);
}

/* --------------------------------------------------------------------------
 * One byte, MSB first, mode 0
 * -------------------------------------------------------------------------- */
static uint8_t xfer_byte(uint8_t out)
{
    uint8_t in = 0;
    int8_t  b;

    for (b = 7; b >= 0; b--)
    {
        /* set MOSI while the clock is low */
        if ((out >> b) & 1u)
        {
            ORION_PIN_SDO_Set();
        }
        else
        {
            ORION_PIN_SDO_Clear();
        }

        half_bit();

        /* rising edge - slave samples here */
        ORION_PIN_SCK_Set();
        half_bit();

        /* sample MISO while the clock is still high */
        in = (uint8_t) ((in << 1) | (ORION_PIN_SDI_Get() ? 1u : 0u));

        /* falling edge */
        ORION_PIN_SCK_Clear();
    }

    return in;
}

/* --------------------------------------------------------------------------
 * Burst with CS held low for the WHOLE transfer
 * --------------------------------------------------------------------------
 * This is the reason hardware SS cannot be used: an ORION frame is
 *      byte0 = R/W | BDST | slave<<1 | addr[8]
 *      byte1 = addr[7:0]
 *      byte2 = data (or dummy on a read; the answer is the 3rd MISO byte)
 * and CS must stay asserted across all three.
 */
void spi_hw_xfer(const uint8_t *tx, uint8_t *rx, uint16_t len)
{
    uint16_t i;

    ORION_PIN_CS_Clear();               /* assert nCS (active low) */
    spi_hw_delay_us(ORION_CS_SETUP_US);

    for (i = 0; i < len; i++)
    {
        uint8_t got = xfer_byte(tx[i]);
        if (rx != 0)
        {
            rx[i] = got;
        }
    }

    spi_hw_delay_us(ORION_CS_SETUP_US);
    ORION_PIN_CS_Set();                 /* deassert */

    /* Guarantee the minimum gap before the next frame. */
    spi_hw_delay_us(ORION_CS_SETUP_US);
}

/* --------------------------------------------------------------------------
 * Control lines
 * -------------------------------------------------------------------------- */
void spi_hw_set_tr(bool level)
{
#if ORION_HAS_TR
    if (level) ORION_PIN_TR_Set(); else ORION_PIN_TR_Clear();
#else
    (void) level;
#endif
}

void spi_hw_set_pa(bool level)
{
#if ORION_HAS_PA
    if (level) ORION_PIN_PA_Set(); else ORION_PIN_PA_Clear();
#else
    (void) level;
#endif
}

void spi_hw_set_txl(bool level)
{
    (void) level;                       /* not wired on this board */
}

void spi_hw_set_rxl(bool level)
{
    (void) level;                       /* not wired on this board */
}

/* --------------------------------------------------------------------------
 * Bring-up helper
 * --------------------------------------------------------------------------
 * Distinguishes the two failure modes that look identical from a distance:
 *   bit0 set -> every read came back 0xFF (MISO stuck high / floating)
 *   bit1 set -> every read came back 0x00 (no device, or MISO tied low)
 * Both zero means the bus is toggling and the answer is simply wrong.
 */
uint8_t spi_hw_selftest(void)
{
    static const uint16_t probe[4] = { 0x000, 0x001, 0x012, 0x0A6 };
    uint8_t flags = 0x03;               /* assume both, clear as disproved */
    uint8_t i;

    for (i = 0; i < 4u; i++)
    {
        uint8_t tx[3], rx[3];
        tx[0] = (uint8_t) (0xBE | ((probe[i] >> 8) & 1u));  /* read, default slave */
        tx[1] = (uint8_t) (probe[i] & 0xFFu);
        tx[2] = 0x00;

        spi_hw_xfer(tx, rx, 3);

        if (rx[2] != 0xFFu) flags &= (uint8_t) ~0x01u;
        if (rx[2] != 0x00u) flags &= (uint8_t) ~0x02u;
    }

    return flags;
}
