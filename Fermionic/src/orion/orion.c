/*
 * orion.c - ORION beamformer driver core
 *
 * Hardware independent: everything physical goes through spi_hw.h, so this
 * file compiles unchanged on the dsPIC and on a host PC for unit testing.
 */

#include "orion.h"
#include "spi_hw.h"

/* Current T/R state, mirrored so stg2_load() knows which strobe to pulse. */
static orion_trx_t s_trx = ORION_MODE_RX;

/* Width of the stage-2 load strobe. */
#define ORION_STROBE_US     2u

/* -------------------------------------------------------------------------- */
void orion_init(orion_t *dev, uint8_t slave)
{
    if (dev == 0) return;
    dev->slave     = (slave <= ORION_SLV_MAX) ? slave : ORION_SLV_DEFAULT;
    dev->device_id = 0;
    dev->major_rev = 0;
    dev->minor_rev = 0;
    dev->present   = false;
}

/* -------------------------------------------------------------------------- */
orion_status_t orion_write(orion_t *dev, uint16_t addr, uint8_t data)
{
    uint8_t frame[ORION_FRAME_LEN];

    if (dev == 0 || addr > ORION_ADDR_MAX) return ORION_ERR_ARG;

    orion_frame_write(frame, dev->slave, addr, data);
    spi_hw_xfer(frame, 0, ORION_FRAME_LEN);
    return ORION_OK;
}

/* -------------------------------------------------------------------------- */
orion_status_t orion_read(orion_t *dev, uint16_t addr, uint8_t *out)
{
    uint8_t frame[ORION_FRAME_LEN];
    uint8_t rx[ORION_FRAME_LEN];

    if (dev == 0 || out == 0 || addr > ORION_ADDR_MAX) return ORION_ERR_ARG;

    orion_frame_read(frame, dev->slave, addr);
    spi_hw_xfer(frame, rx, ORION_FRAME_LEN);

    /* The device returns the register contents in the third byte, matching
     * the host driver's rx[2]. */
    *out = rx[2];
    return ORION_OK;
}

/* -------------------------------------------------------------------------- */
orion_status_t orion_write_verify(orion_t *dev, uint16_t addr, uint8_t data)
{
    uint8_t got;
    orion_status_t st = orion_write(dev, addr, data);
    if (st != ORION_OK) return st;

    st = orion_read(dev, addr, &got);
    if (st != ORION_OK) return st;

    return (got == data) ? ORION_OK : ORION_ERR_VERIFY;
}

/* -------------------------------------------------------------------------- */
orion_status_t orion_broadcast(uint16_t addr, uint8_t data)
{
    uint8_t frame[ORION_FRAME_LEN];

    if (addr > ORION_ADDR_MAX) return ORION_ERR_ARG;

    /* Broadcast is write-only by construction: every device on the bus would
     * drive MISO simultaneously on a read. */
    orion_frame_write(frame, ORION_SLV_BROADCAST, addr, data);
    spi_hw_xfer(frame, 0, ORION_FRAME_LEN);
    return ORION_OK;
}

/* -------------------------------------------------------------------------- */
orion_status_t orion_rmw(orion_t *dev, uint16_t addr,
                         uint8_t lsb, uint8_t width, uint8_t value)
{
    uint8_t cur, mask, next;
    orion_status_t st;

    if (dev == 0 || width == 0u || width > 8u || (uint16_t)lsb + width > 8u)
        return ORION_ERR_ARG;

    st = orion_read(dev, addr, &cur);
    if (st != ORION_OK) return st;

    mask = (uint8_t)(((width >= 8u) ? 0xFFu : (uint8_t)((1u << width) - 1u)) << lsb);
    next = (uint8_t)((cur & (uint8_t)~mask) | (uint8_t)((value << lsb) & mask));

    return orion_write(dev, addr, next);
}

/* -------------------------------------------------------------------------- */
orion_status_t orion_probe(orion_t *dev)
{
    orion_status_t st;
    uint8_t id, rev;

    if (dev == 0) return ORION_ERR_ARG;
    dev->present = false;

    st = orion_read(dev, ORION_REG_DEVICE_ID, &id);
    if (st != ORION_OK) return st;
    dev->device_id = id;

    st = orion_read(dev, ORION_REG_REVISION, &rev);
    if (st != ORION_OK) return st;
    dev->major_rev = (uint8_t)(rev & 0x0Fu);
    dev->minor_rev = (uint8_t)((rev >> 4) & 0x0Fu);

    if (id != ORION_DEVICE_ID_EXPECTED) return ORION_ERR_ID;

    /* A revision other than 1.1 is reported by the caller as a warning, not a
     * failure - new silicon spins must still be usable. */
    dev->present = true;
    return ORION_OK;
}

/* -------------------------------------------------------------------------- */
orion_status_t orion_sync_reset(orion_t *dev)
{
    orion_status_t st;
    if (dev == 0) return ORION_ERR_ARG;

    /* SPI_MODE_CTRL.sync_rst is bit 0 per the register map. */
    st = orion_rmw(dev, ORION_REG_SPI_MODE_CTRL, 0, 1, 1);
    if (st != ORION_OK) return st;
    spi_hw_delay_us(10);
    return orion_rmw(dev, ORION_REG_SPI_MODE_CTRL, 0, 1, 0);
}

/* -------------------------------------------------------------------------- */
orion_status_t orion_write_block(orion_t *dev, uint16_t start_addr,
                                 const uint8_t *data, uint16_t len)
{
    uint16_t i;
    orion_status_t st;

    if (dev == 0 || data == 0) return ORION_ERR_ARG;
    if ((uint32_t)start_addr + len > (uint32_t)ORION_ADDR_MAX + 1u)
        return ORION_ERR_ARG;

    /* One frame per byte, streamed. No intermediate buffer: the LUT memories
     * are up to 256 bytes and this MCU only has 4 KB of RAM total. */
    for (i = 0; i < len; i++) {
        st = orion_write(dev, (uint16_t)(start_addr + i), data[i]);
        if (st != ORION_OK) return st;
    }
    return ORION_OK;
}

/* -------------------------------------------------------------------------- */
orion_status_t orion_write_burst(orion_t *dev, uint16_t start_addr,
                                 const uint8_t *data, uint16_t len,
                                 uint8_t *scratch, uint16_t scratch_len)
{
    uint16_t i;

    if (dev == 0 || data == 0 || scratch == 0) return ORION_ERR_ARG;
    if (len == 0u) return ORION_OK;
    if ((uint32_t)start_addr + len > (uint32_t)ORION_ADDR_MAX + 1u)
        return ORION_ERR_ARG;
    if (scratch_len < (uint16_t)(len + 2u)) return ORION_ERR_ARG;

    /*
     * Header is the usual byte0/byte1, then the payload streams out while CS
     * stays low. The device increments its own address pointer per the
     * programming guide, so no further headers are sent.
     */
    scratch[0] = (uint8_t)(ORION_RW_WRITE
                           | (uint8_t)((dev->slave & 0x1Fu) << 1)
                           | (uint8_t)((start_addr >> 8) & 0x01u));
    scratch[1] = (uint8_t)(start_addr & 0xFFu);
    for (i = 0; i < len; i++) scratch[2 + i] = data[i];

    spi_hw_xfer(scratch, 0, (uint16_t)(len + 2u));
    return ORION_OK;
}

/* --- per-channel helpers --------------------------------------------------
 * Register layout is a regular stride of 2 from the channel-0 address:
 *   PHASE_CODE_TX0 = 0x02, GAIN_CODE_TX0 = 0x03, PHASE_CODE_TX1 = 0x04, ...
 *   PHASE_CODE_RX0 = 0x0A, GAIN_CODE_RX0 = 0x0B, PHASE_CODE_RX1 = 0x0C, ...
 */
orion_status_t orion_set_phase_tx(orion_t *dev, uint8_t ch, uint8_t code)
{
    if (ch > 3u) return ORION_ERR_ARG;
    return orion_write(dev, (uint16_t)(ORION_REG_PHASE_TX0 + (uint16_t)ch * 2u),
                       (uint8_t)(code & 0x7Fu));      /* 7-bit field */
}

orion_status_t orion_set_gain_tx(orion_t *dev, uint8_t ch, uint8_t code)
{
    if (ch > 3u) return ORION_ERR_ARG;
    return orion_write(dev, (uint16_t)(ORION_REG_GAIN_TX0 + (uint16_t)ch * 2u),
                       (uint8_t)(code & 0x3Fu));      /* 6-bit field */
}

orion_status_t orion_set_phase_rx(orion_t *dev, uint8_t ch, uint8_t code)
{
    if (ch > 3u) return ORION_ERR_ARG;
    return orion_write(dev, (uint16_t)(ORION_REG_PHASE_RX0 + (uint16_t)ch * 2u),
                       (uint8_t)(code & 0x7Fu));
}

orion_status_t orion_set_gain_rx(orion_t *dev, uint8_t ch, uint8_t code)
{
    if (ch > 3u) return ORION_ERR_ARG;
    return orion_write(dev, (uint16_t)(ORION_REG_GAIN_RX0 + (uint16_t)ch * 2u),
                       (uint8_t)(code & 0x3Fu));
}

orion_status_t orion_set_beam(orion_t *dev, uint8_t beam_idx)
{
    return orion_write(dev, ORION_REG_BEAM_CODE, beam_idx);
}

/* --- control lines --------------------------------------------------------- */
void orion_trx_set(orion_trx_t mode)
{
    s_trx = mode;
    spi_hw_set_tr(mode == ORION_MODE_TX);
}

orion_trx_t orion_trx_get(void) { return s_trx; }

void orion_pa_enable(bool on) { spi_hw_set_pa(on); }

void orion_txl_strobe(void)
{
    spi_hw_set_txl(true);
    spi_hw_delay_us(ORION_STROBE_US);
    spi_hw_set_txl(false);
}

void orion_rxl_strobe(void)
{
    spi_hw_set_rxl(true);
    spi_hw_delay_us(ORION_STROBE_US);
    spi_hw_set_rxl(false);
}

void orion_stg2_load(void)
{
    if (s_trx == ORION_MODE_TX) orion_txl_strobe();
    else                        orion_rxl_strobe();
}

orion_status_t orion_update_code_pulse(orion_t *dev)
{
    orion_status_t st;

    /* Mirrors the host driver: UPDATE_CODE 0 -> 1 -> 0. */
    st = orion_write(dev, ORION_REG_UPDATE_CODE, 0x00);
    if (st != ORION_OK) return st;
    st = orion_write(dev, ORION_REG_UPDATE_CODE, 0x01);
    if (st != ORION_OK) return st;
    return orion_write(dev, ORION_REG_UPDATE_CODE, 0x00);
}
