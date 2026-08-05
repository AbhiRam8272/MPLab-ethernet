/*
 * orion.h - ORION beamformer driver API (dsPIC30F5011, standalone SPI master)
 */

#ifndef ORION_H
#define ORION_H

#include <stdint.h>
#include <stdbool.h>
#include "orion_proto.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Result codes ------------------------------------------------------------ */
typedef enum {
    ORION_OK = 0,
    ORION_ERR_ARG,          /* bad address / slave / parameter                */
    ORION_ERR_VERIFY,       /* register read-back did not match what we wrote */
    ORION_ERR_ID,           /* DEVICE_ID mismatch                             */
    ORION_ERR_TIMEOUT       /* SPI peripheral did not complete                */
} orion_status_t;

/* T/R state --------------------------------------------------------------- */
typedef enum { ORION_MODE_RX = 0, ORION_MODE_TX = 1 } orion_trx_t;

/* Device handle ----------------------------------------------------------- */
typedef struct {
    uint8_t  slave;         /* ORION_SLV_DEFAULT unless multi-chip addressing */
    uint8_t  device_id;     /* filled by orion_probe()                        */
    uint8_t  major_rev;
    uint8_t  minor_rev;
    bool     present;       /* true once probe has succeeded                  */
} orion_t;

/* --- lifecycle ------------------------------------------------------------ */
void            orion_init(orion_t *dev, uint8_t slave);
orion_status_t  orion_probe(orion_t *dev);
orion_status_t  orion_sync_reset(orion_t *dev);

/* --- raw register access -------------------------------------------------- */
orion_status_t  orion_write(orion_t *dev, uint16_t addr, uint8_t data);
orion_status_t  orion_read (orion_t *dev, uint16_t addr, uint8_t *out);
orion_status_t  orion_write_verify(orion_t *dev, uint16_t addr, uint8_t data);
orion_status_t  orion_broadcast(uint16_t addr, uint8_t data);

/* Read-modify-write of a bit field. lsb 0..7, width 1..8. */
orion_status_t  orion_rmw(orion_t *dev, uint16_t addr,
                          uint8_t lsb, uint8_t width, uint8_t value);

/* --- bulk / streaming ----------------------------------------------------- */
/*
 * Stream a block of bytes to consecutive addresses. Used for the LUT memories
 * at 256..511. Streams one byte at a time so no RAM buffer is needed - the
 * dsPIC30F5011 only has 4 KB and a full RX phase LUT does not fit.
 */
orion_status_t  orion_write_block(orion_t *dev, uint16_t start_addr,
                                  const uint8_t *data, uint16_t len);

/*
 * True SPI block write (programming guide 2.3.1): a single CS assertion sends
 * the 2-byte header once and then N data bytes, with the device
 * auto-incrementing its address pointer. Roughly 3x fewer SPI bytes than one
 * framed transaction per register.
 *
 * Requires a contiguous address range and a caller-supplied scratch buffer of
 * at least len + 2 bytes (this MCU has no heap).
 */
orion_status_t  orion_write_burst(orion_t *dev, uint16_t start_addr,
                                  const uint8_t *data, uint16_t len,
                                  uint8_t *scratch, uint16_t scratch_len);

/* --- beam / channel control ----------------------------------------------- */
orion_status_t  orion_set_phase_tx(orion_t *dev, uint8_t ch, uint8_t code);
orion_status_t  orion_set_gain_tx (orion_t *dev, uint8_t ch, uint8_t code);
orion_status_t  orion_set_phase_rx(orion_t *dev, uint8_t ch, uint8_t code);
orion_status_t  orion_set_gain_rx (orion_t *dev, uint8_t ch, uint8_t code);
orion_status_t  orion_set_beam(orion_t *dev, uint8_t beam_idx);

/* --- hardware control lines (replace the USB bridge's GPIO expander) ------ */
void            orion_trx_set(orion_trx_t mode);
orion_trx_t     orion_trx_get(void);
void            orion_pa_enable(bool on);
void            orion_txl_strobe(void);     /* pulse TX stage-2 load          */
void            orion_rxl_strobe(void);     /* pulse RX stage-2 load          */
void            orion_stg2_load(void);      /* strobe whichever path is active */

/* Register-driven stage-2 update (UPDATE_CODE 0 -> 1 -> 0), the equivalent of
 * the host driver's stg2_load() when STG2_CFG selects register control. */
orion_status_t  orion_update_code_pulse(orion_t *dev);

#ifdef __cplusplus
}
#endif
#endif /* ORION_H */
