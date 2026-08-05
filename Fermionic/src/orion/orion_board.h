/*
 * orion_board.h - ORION beamformer wiring for the UDIGPSB0183 board
 * ===========================================================================
 * Written against YOUR project (AbhiRam8272/MPLab-ethernet, commit e12260a),
 * not against a Microchip reference board. Every pin below was checked against
 * src/config/default/peripheral/gpio/plib_gpio.h in that project.
 *
 * ---------------------------------------------------------------------------
 * WHY THE OBVIOUS SPI PINS DO NOT WORK HERE
 * ---------------------------------------------------------------------------
 * Your app already drives 68 GPIOs (8 phase shifters x 6 bits, 8 enables,
 * 9 switches, fan SSR). Three of them are the pins normally used for SPI1:
 *
 *      RD0  = PS3_5      (also SCK_HIGH/SCK_LOW in app.h - LMX bit-bang)
 *      RD9  = PS4_1      (also LE_1_HIGH/LE_1_LOW      - LMX bit-bang)
 *      RD10 = PS3_6      (also SDO_HIGH/SDO_LOW        - LMX bit-bang)
 *
 * So the SPI1 pinout proposed for the Digilent board is NOT available.
 * Note also that app.h and plib_gpio.h disagree about RD0/RD9/RD10: the
 * bit-bang macros in app.h write LATD directly while plib_gpio.h assigns the
 * same bits to PS3_5 / PS4_1 / PS3_6. Worth resolving on your side.
 *
 * SPI2 is unusable for a different reason: RG8/RG9 are Ethernet.
 *
 * ---------------------------------------------------------------------------
 * FREE PINS ON YOUR BOARD (computed from your plib_gpio.h)
 * ---------------------------------------------------------------------------
 * Only these remain unassigned:
 *
 *      RA15  RC12  RD8  RD11  RD14  RD15  RE8  RE9  RF2  RF6  RF8
 *      RG2   RG3   RG8  RG9   RG15
 *
 * Of those, several are already spoken for by silicon function:
 *      RC12, RG15 ....... OSC1/secondary oscillator area (POSCMOD = EC)
 *      RG2,  RG3 ........ USB D+/D- (FVBUSONIO = ON, FUSBIDIO = ON)
 *      RG8,  RG9 ........ Ethernet RMII
 *      RD8,  RD11 ....... Ethernet alternate set (FETHIO = OFF) - see below
 *
 * That leaves a clean, genuinely free set:
 *
 *      RD14  RD15  RE8  RE9  RF2  RF6  RF8  RA15
 *
 * ---------------------------------------------------------------------------
 * CHOSEN WIRING - bit-banged SPI on free pins
 * ---------------------------------------------------------------------------
 * There is no free hardware-SPI pin group left, so ORION uses a software SPI
 * master. That is not a compromise for this part: an ORION frame is only
 * 3 bytes and the LUT load is 1920 bytes, so even a slow bit-bang finishes a
 * full LUT in well under a second, and mode-0 timing is exact by construction.
 *
 *      Signal    Pin    Notes
 *      -------   ----   -----------------------------------------------
 *      SCK       RD14   idle low (mode 0)
 *      SDO       RD15   MCU -> ORION  (MOSI)
 *      SDI       RE8    ORION -> MCU  (MISO), input
 *      nCS       RE9    held LOW across all 3 bytes of a frame
 *      TR        RF2    T/R switch     (optional)
 *      PA_EN     RF8    PA enable      (optional)
 *
 * >>> ACTION REQUIRED IN MHC <<<
 * Add these six pins in the Pin Settings tab with EXACTLY these custom names:
 *
 *      RD14 -> ORION_SCK   output
 *      RD15 -> ORION_SDO   output
 *      RE8  -> ORION_SDI   INPUT
 *      RE9  -> ORION_CS    output, initial value HIGH
 *      RF2  -> ORION_TR    output
 *      RF8  -> ORION_PA    output
 *
 * If you prefer different pins, change only the six names below - no .c file
 * needs to be touched.
 *
 * ---------------------------------------------------------------------------
 * ETHERNET IS ALREADY WORKING IN YOUR PROJECT - DO NOT CHANGE IT
 * ---------------------------------------------------------------------------
 * Verified in your config/default:
 *      TCPIP_STACK_USE_BERKELEY_API   already defined  (configuration.h:301)
 *      MAX_BSD_SOCKETS                4                (configuration.h:302)
 *      TCPIP_TCP_MAX_SOCKETS          10               (configuration.h:228)
 *      FETHIO = OFF  FMIIEN = OFF     alternate RMII pins
 *      IP 10.0.0.191 / 255.0.0.0 / gw 10.0.0.1
 *
 * Berkeley sockets are ALREADY enabled - nothing to turn on. This is the one
 * piece that usually has to be added by hand, and you already have it.
 *
 * ---------------------------------------------------------------------------
 * ETHERNET-ONLY CONTROL - NO UART ANYWHERE IN THE ORION PATH
 * ---------------------------------------------------------------------------
 * Every ORION function - registers, beam steering, the epsilon vendor tests
 * and bulk LUT loading - arrives on TCP port ORION_TCP_PORT. There is no
 * serial command path and no serial output path.
 *
 * Your project currently has UART1_Initialize() and SYS_CONSOLE on UART1.
 * ORION does NOT use either. You may delete both from MHC; app.c defines
 * no-op fallbacks for SYS_CONSOLE_MESSAGE / SYS_CONSOLE_PRINT so the nine
 * legacy calls in your own code still compile. Verified by building with
 * SYS_CONSOLE completely undefined.
 *
 * Start-up diagnostics (SPI self-test, DEVICE_ID, link state, IP address) run
 * before any client can connect, so they are buffered in a 1 KB RAM log and
 * replayed to the first TCP client. The 'boot' command reprints them.
 *
 * ---------------------------------------------------------------------------
 * TWO SPI BACKENDS - PICK ONE
 * ---------------------------------------------------------------------------
 * You said you would set up SPI yourself, so both options are provided:
 *
 *   orion_spi_udig.c   bit-bang on the six free pins below.  <-- default,
 *                      works with your board exactly as it ships today.
 *
 *   orion_spi_plib.c   Harmony SPI PLIB. Use this if you free up SPI1 or
 *                      SPI2. Edit the three ORION_SPI_* macros at the top of
 *                      that file to name your instance.
 *
 * They define the same symbols, so EXACTLY ONE may be in the build.
 * ./check_build.sh enforces this.
 */

#ifndef ORION_BOARD_H
#define ORION_BOARD_H

#include <stdint.h>
#include <stdbool.h>

/* --------------------------------------------------------------------------
 * Pin names (MHC custom names -> generated macros in plib_gpio.h)
 * -------------------------------------------------------------------------- */
//#define ORION_PIN_SCK_Set()         ORION_SCK_Set()
//#define ORION_PIN_SCK_Clear()       ORION_SCK_Clear()
//#define ORION_PIN_SDO_Set()         ORION_SDO_Set()
//#define ORION_PIN_SDO_Clear()       ORION_SDO_Clear()
//#define ORION_PIN_SDI_Get()         ORION_SDI_Get()
//#define ORION_PIN_CS_Set()          ORION_CS_Set()
//#define ORION_PIN_CS_Clear()        ORION_CS_Clear()

/* Optional control lines. Set to 0 if you did not wire them. */
//#define ORION_HAS_TR                1
//#define ORION_PIN_TR_Set()          ORION_TR_Set()
//#define ORION_PIN_TR_Clear()        ORION_TR_Clear()

#define ORION_HAS_PA                1
//#define ORION_PIN_PA_Set()          ORION_PA_Set()
//#define ORION_PIN_PA_Clear()        ORION_PA_Clear()

/*
 * Your board has no separate RX/TX LNA lines wired to the PIC, so the driver's
 * rxl/txl helpers are compiled out. orion.c handles this.
 */
//#define ORION_HAS_RXL               0
//#define ORION_HAS_TXL               0

/*
 * PHY reset is NOT driven here. Your Ethernet already links with the stack's
 * own PHY handling, so touching it would be a regression. Left at 0
 * deliberately.
 */
//#define ORION_DRIVE_PHY_RESET       0

/*
 * Heartbeat: your basic_app_main.c already toggles EN_1..EN_8 once a second
 * from TMR1. ORION does not add its own LED.
 */
//#define ORION_HAS_LED               0

/* --------------------------------------------------------------------------
 * Bit-bang SPI timing
 * --------------------------------------------------------------------------
 * SYSCLK is 80 MHz (FNOSC=PRIPLL, FPLLIDIV=DIV_2, FPLLMUL=MUL_20,
 * FPLLODIV=DIV_1) and FPBDIV=DIV_1, both confirmed in your initialization.c.
 *
 * ORION_SPI_HALF_US is the half-period. 1 us -> ~500 kHz, which is far below
 * the 25 MHz ceiling in the FD3R4411 programming guide and safe on flying
 * leads. Drop it to 0 for maximum speed once the link is proven on a scope.
 */
#define ORION_SPI_HALF_US           1
#define ORION_CS_SETUP_US           1
#define ORION_STROBE_US             2

/* --------------------------------------------------------------------------
 * Networking
 * --------------------------------------------------------------------------
 * Your existing app listens on 5001. ORION uses a SEPARATE port so both can
 * run side by side - your phase-shifter/IO protocol is untouched.
 */
#define ORION_TCP_PORT              2323

#define ORION_RX_CHUNK              512
#define ORION_TX_BUF                2048

#endif /* ORION_BOARD_H */
