/*
 * orion_spi_plib.c - spi_hw.h on a Harmony SPI PLIB (ALTERNATIVE to bit-bang)
 * ===========================================================================
 * You said you would configure SPI yourself. This is the file to use if you
 * free up a hardware SPI block; keep orion_spi_udig.c if you stay on the
 * bit-bang pins.
 *
 *   >>> USE EXACTLY ONE OF THESE TWO FILES. <<<
 *   Both define the same spi_hw_* symbols, so including both in the MPLAB
 *   project gives "multiple definition" at link time.
 *
 *   Default in configurations.xml: orion_spi_udig.c (bit-bang).
 *   To switch: right-click orion_spi_udig.c -> Properties -> Exclude from
 *   build, and add this file instead.
 *
 * ---------------------------------------------------------------------------
 * MHC SETTINGS THIS FILE ASSUMES
 * ---------------------------------------------------------------------------
 *   Master mode
 *   8-bit
 *   CPOL = 0, CPHA = 0      (SPI mode 0)      <-- ORION requires mode 0
 *   MSB first
 *   Baud <= 25 MHz          (FD3R4411 programming guide limit)
 *                           start at 1 MHz, raise once proven on a scope
 *
 *   >>> HARDWARE SLAVE SELECT MUST BE **OFF** <<<
 *   An ORION frame is 3 bytes inside ONE chip-select assertion:
 *       byte0 = R/W | BDST | slave<<1 | addr[8]
 *       byte1 = addr[7:0]
 *       byte2 = data   (or dummy on a read; answer is the 3rd MISO byte)
 *   PIC32 hardware SS pulses CS between bytes and breaks every transaction.
 *   CS is therefore driven as a plain GPIO below.
 *
 * ---------------------------------------------------------------------------
 * WHICH SPI INSTANCE?
 * ---------------------------------------------------------------------------
 * On YOUR board neither SPI1 nor SPI2 is free as shipped:
 *     SPI1  RD0/RD9/RD10   -> PS3_5 / PS4_1 / PS3_6  (+ LMX bit-bang in app.h)
 *     SPI2  RG6/RG7        -> PS7_5 / PS7_4
 *           RG8/RG9        -> Ethernet RMII (ECRSDV / EREFCLK) - never usable
 * So you must first free one of those pin groups. Change the three macros
 * below to match whichever instance you enable in MHC.
 */

#include <stdint.h>
#include <stdbool.h>

#include "definitions.h"
#include "spi_hw.h"
#include "orion_board.h"

/* --------------------------------------------------------------------------
 * Which PLIB instance - EDIT THESE THREE LINES
 * -------------------------------------------------------------------------- */
#ifndef ORION_SPI_WriteRead
#define ORION_SPI_Initialize()          SPI1_Initialize()
#define ORION_SPI_WriteRead(t,tl,r,rl)  SPI1_WriteRead((void*)(t),(tl),(void*)(r),(rl))
#define ORION_SPI_IsBusy()              SPI1_IsTransmitterBusy()
#endif

/* --------------------------------------------------------------------------
 * Delay - CP0 Count runs at SYSCLK/2 = 40 MHz on your 80 MHz part
 * -------------------------------------------------------------------------- */
#define CP0_TICKS_PER_US    (CPU_CLOCK_FREQUENCY / 2000000UL)

void spi_hw_delay_us(uint16_t us)
{
    uint32_t start = _CP0_GET_COUNT();
    uint32_t want  = (uint32_t) us * CP0_TICKS_PER_US;

    /* Unsigned subtraction - correct across the 32-bit wrap. */
    while ((_CP0_GET_COUNT() - start) < want)
    {
        /* spin */
    }
}

void spi_hw_init(void)
{
    /*
     * SYS_Initialize() already called the PLIB's own Initialize(). This only
     * establishes the idle state of the GPIO-driven lines.
     */
    ORION_PIN_CS_Set();                 /* nCS idle HIGH = deselected */

#if ORION_HAS_TR
    ORION_PIN_TR_Clear();               /* safe idle: RX */
#endif
#if ORION_HAS_PA
    ORION_PIN_PA_Clear();               /* PA off */
#endif

    spi_hw_delay_us(10);
}

/* --------------------------------------------------------------------------
 * Burst with CS held low for the WHOLE transfer
 * -------------------------------------------------------------------------- */
void spi_hw_xfer(const uint8_t *tx, uint8_t *rx, uint16_t len)
{
    static uint8_t scratch[8];          /* used when the caller passes rx=NULL */
    uint8_t *dst = rx;

    if (dst == 0)
    {
        dst = scratch;                  /* PLIB still needs somewhere to land */
    }

    ORION_PIN_CS_Clear();               /* assert nCS (active low) */
    spi_hw_delay_us(ORION_CS_SETUP_US);

    (void) ORION_SPI_WriteRead(tx, len, dst, len);

    /* Blocking PLIB call, but drain anyway in case a non-blocking driver is
     * configured - releasing CS early would corrupt the frame. */
    while (ORION_SPI_IsBusy())
    {
        /* spin */
    }

    spi_hw_delay_us(ORION_CS_SETUP_US);
    ORION_PIN_CS_Set();                 /* deassert */
    spi_hw_delay_us(ORION_CS_SETUP_US); /* minimum gap before the next frame */
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

void spi_hw_set_txl(bool level) { (void) level; }   /* not wired */
void spi_hw_set_rxl(bool level) { (void) level; }   /* not wired */

/* --------------------------------------------------------------------------
 * Bring-up helper - same contract as the bit-bang version
 *   bit0 -> every read returned 0xFF (MISO floating / stuck high)
 *   bit1 -> every read returned 0x00 (no device / MISO tied low)
 *   0    -> bus toggles but the answer is wrong (mode? clock? wiring?)
 * -------------------------------------------------------------------------- */
uint8_t spi_hw_selftest(void)
{
    static const uint16_t probe[4] = { 0x000, 0x001, 0x012, 0x0A6 };
    uint8_t flags = 0x03;
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
