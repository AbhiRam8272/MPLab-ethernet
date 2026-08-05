/*
 * orion_proto.h - ORION beamformer SPI wire protocol (portable, no MCU deps)
 * ---------------------------------------------------------------------------
 * Every ORION register access is a 3-byte, chip-select-framed SPI burst:
 *
 *      byte0 = RW | (slave_addr << 1) | addr[8]
 *      byte1 = addr[7:0]
 *      byte2 = write data   (or 0x00 dummy on a read)
 *
 * On a read the device returns the register contents in the 3rd MISO byte.
 *
 * Derived from the reference host driver (include/SPI.py):
 *      write default    : [0x3E | a8, a7_0, data]      slave 0x1F
 *      write broadcast  : [0x40 | a8, a7_0, data]      slave 0x20
 *      write addressed  : [(slv<<1) | a8, a7_0, data]
 *      read  default    : [0xBE | a8, a7_0, 0x00]      0x80 | 0x3E
 *      read  addressed  : [0x80 | (slv<<1) | a8, a7_0, 0x00]
 *
 * NOTE the 9-bit address. CSRs occupy 0..211, but the gain/phase/beam LUT
 * memories live at 256..511, so addr[8] must be carried in byte0. Truncating
 * the address to 8 bits silently corrupts every LUT write.
 *
 * This header is deliberately free of any dsPIC/XC16 dependency so the frame
 * encoder can be unit-tested on a host PC.
 */

#ifndef ORION_PROTO_H
#define ORION_PROTO_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* --- transaction geometry ------------------------------------------------ */
#define ORION_FRAME_LEN         3u

/* --- slave addressing ----------------------------------------------------- */
/*
 * Byte 0 layout, per the FD3R4411 programming guide section 2.2:
 *
 *      bit 7    R/W_n
 *      bit 6    BDST (broadcast)
 *      bits 5:1 5-bit SPI slave address   <-- FIVE bits, not six
 *      bit 0    addr[8]
 *
 * The ADDR0-4 straps give 32 addressable devices on one bus. A "slave" value
 * above 0x1F would overflow into the BDST bit and silently turn every write
 * into a broadcast, so the cap is 0x1F.
 */
#define ORION_SLV_DEFAULT       0x1Fu   /* 0x1F<<1 = 0x3E : all-ones strap          */
#define ORION_SLV_BROADCAST     0x20u   /* sets BDST; handled as a pseudo-address   */
#define ORION_SLV_MAX           0x1Fu   /* 5 bits                                   */

/* --- direction bit -------------------------------------------------------- */
#define ORION_RW_WRITE          0x00u
#define ORION_RW_READ           0x80u

/* --- address space -------------------------------------------------------- */
#define ORION_ADDR_MAX          0x1FFu  /* 9-bit                                   */
#define ORION_CSR_LAST          211u    /* highest control/status register         */
#define ORION_LUT_BASE          256u    /* TX/RX phase, gain and beam memories      */

/* --- known registers (from regs/ORION_8G_12G_csr.csv) --------------------- */
#define ORION_REG_DEVICE_ID     0x000u
#define ORION_REG_REVISION      0x001u
#define ORION_REG_PHASE_TX0     0x002u
#define ORION_REG_GAIN_TX0      0x003u
#define ORION_REG_PHASE_RX0     0x00Au
#define ORION_REG_GAIN_RX0      0x00Bu
#define ORION_REG_UPDATE_CODE   0x012u
#define ORION_REG_BEAM_CODE     0x014u
#define ORION_REG_BEAM_CFG      0x016u
#define ORION_REG_STG2_CFG      0x017u
#define ORION_REG_FREQ_ID       0x018u
#define ORION_REG_CORR_CFG      0x01Bu
#define ORION_REG_SPI_MODE_CTRL 0x01Du

#define ORION_DEVICE_ID_EXPECTED 0xF2u
#define ORION_MAJOR_REV_EXPECTED 0x1u
#define ORION_MINOR_REV_EXPECTED 0x1u

/*
 * Build the 3-byte frame for a register write.
 *   out       : caller-supplied buffer, at least ORION_FRAME_LEN bytes
 *   slave     : ORION_SLV_DEFAULT / ORION_SLV_BROADCAST / 0..0x3F
 *   addr      : 9-bit register address
 *   data      : byte to write
 */
static inline void orion_frame_write(uint8_t *out, uint8_t slave,
                                     uint16_t addr, uint8_t data)
{
    out[0] = (uint8_t)(ORION_RW_WRITE
                       | (uint8_t)((slave & 0x3Fu) << 1)
                       | (uint8_t)((addr >> 8) & 0x01u));
    out[1] = (uint8_t)(addr & 0xFFu);
    out[2] = data;
}

/*
 * Build the 3-byte frame for a register read. The third byte is a dummy the
 * device clocks its answer out over.
 */
static inline void orion_frame_read(uint8_t *out, uint8_t slave, uint16_t addr)
{
    out[0] = (uint8_t)(ORION_RW_READ
                       | (uint8_t)((slave & 0x3Fu) << 1)
                       | (uint8_t)((addr >> 8) & 0x01u));
    out[1] = (uint8_t)(addr & 0xFFu);
    out[2] = 0x00u;
}

#ifdef __cplusplus
}
#endif
#endif /* ORION_PROTO_H */
