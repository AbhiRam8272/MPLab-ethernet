/*
 * orion_hal.c - epsilon-variant bring-up sequences (version 'v2')
 *
 * Each function reproduces the register traffic of the corresponding script in
 * orion_sdk/tests/epsilon/, verified against traces captured from the real
 * Python HAL. Comments name the source line where behaviour is non-obvious.
 */

#include "orion_hal.h"
#include "spi_hw.h"

/* Track T/R state so hal_stg2_load() strobes the right line, mirroring the
 * Python HAL's self.trx_mode / self.use_reg_inp_for_stg2. */
static uint8_t s_trx_mode;
static bool    s_stg2_use_reg = true;

#define TRY(x) do { orion_status_t _s = (x); if (_s != ORION_OK) return _s; } while (0)

/* -------------------------------------------------------------------------- */
orion_status_t hal_set_tr_mode(orion_t *d, bool internal)
{
    /* TR_CFG.tr_mode_sel bit 0 */
    return orion_rmw(d, REG_TR_CFG, 0, 1, internal ? 1 : 0);
}

orion_status_t hal_set_trx_mode(orion_t *d, uint8_t tx)
{
    s_trx_mode = tx ? 1u : 0u;
    /* Python only writes TR_SW_CTRL when tr_mode is internal; we mirror that by
     * always writing it, which is what INT_TR mode needs and is harmless in
     * EXT_TR because the pin overrides. */
    return orion_write(d, REG_TR_SW_CTRL, s_trx_mode);
}

orion_status_t hal_set_tr_mask(orion_t *d, int tx_mask, int rx_mask)
{
    uint8_t cur = 0;
    TRY(orion_read(d, REG_TR_MASK, &cur));

    if (tx_mask >= 0) {
        cur = (uint8_t)((cur & 0xF0u) | ((uint8_t)tx_mask & 0x0Fu));
        /* Python's set_tr_mask() also forces TX_LNA_CFG when a tx mask is given. */
        TRY(orion_write(d, REG_TX_LNA_CFG, 0xFF));
    }
    if (rx_mask >= 0) {
        cur = (uint8_t)((cur & 0x0Fu) | (uint8_t)(((uint8_t)rx_mask & 0x0Fu) << 4));
    }
    return orion_write(d, REG_TR_MASK, cur);
}

orion_status_t hal_cfg_stg2_load(orion_t *d, bool use_reg)
{
    s_stg2_use_reg = use_reg;
    return orion_rmw(d, REG_STG2_CFG, 0, 1, use_reg ? 1 : 0);
}

orion_status_t hal_en_data_path(orion_t *d, bool on)
{
    return orion_rmw(d, REG_TR_CFG, 1, 1, on ? 1 : 0);
}

orion_status_t hal_enable_rx_correction(orion_t *d, bool on)
{
    /* CORR_CFG: bit0 en_phase_corr, bit1 en_gain_corr */
    return orion_rmw(d, REG_CORR_CFG, 0, 2, on ? 0x3u : 0x0u);
}

orion_status_t hal_set_freq(orion_t *d, hal_freq_t f)
{
    /* Python set_freq() writes FREQ_ID then re-writes RSVD2 and RSVD3. */
    TRY(orion_write(d, REG_FREQ_ID, (f == HAL_FREQ_11G) ? 1u : 0u));
    TRY(orion_write(d, REG_RSVD2, 0x00));
    return orion_write(d, REG_RSVD3, 0x00);
}

orion_status_t hal_stg2_load(orion_t *d)
{
    if (s_stg2_use_reg) {
        /*
         * Programming guide 4.1.1: set UPDATE_CODE to 1 and then write RSVD0
         * with any value, consecutively. The RSVD0 write is what actually
         * commits the transfer - the reference Python omits it and instead
         * pulses UPDATE_CODE 0/1/0, which happens to work but is not the
         * documented sequence. Both are issued here so either silicon
         * behaviour is satisfied.
         */
        TRY(orion_write(d, REG_UPDATE_CODE, 0));
        TRY(orion_write(d, REG_UPDATE_CODE, 1));
        TRY(orion_write(d, REG_RSVD0, 0x01));
        return orion_write(d, REG_UPDATE_CODE, 0);
    }
    /* Pin-driven: toggle the TXL line, as SPI.txl_tggl() did. */
    spi_hw_set_txl(true);
    spi_hw_delay_us(2);
    spi_hw_set_txl(false);
    return ORION_OK;
}

/* --- per-channel current helpers ----------------------------------------- */
static orion_status_t set_ch_field(orion_t *d, uint16_t base, uint8_t ant_sel,
                                   uint8_t lsb, uint8_t width, uint8_t val)
{
    uint8_t i;
    for (i = 0; i < 4u; i++) {
        if (ant_sel & (1u << i)) {
            TRY(orion_rmw(d, (uint16_t)(base + (uint16_t)i * 2u), lsb, width, val));
        }
    }
    return ORION_OK;
}

/* -------------------------------------------------------------------------- */
orion_status_t hal_init_tx(orion_t *d, hal_bias_t bias, uint8_t final_av,
                           uint8_t ant_sel)
{
    uint8_t i, lna_curr, cmb, drv;

    /* v2 only: REG4_EXT_BIAS.rsvd7 = 0x02. This write is absent on the v1 path
     * and is the main reason epsilon must run as 'v2'. */
    TRY(orion_write(d, REG_REG4_EXT_BIAS, 0x02));

    for (i = 0; i < 4u; i++) {
        if (ant_sel & (1u << i)) {
            /* TX_GAIN_FORCE.tx<i>_final_gain_force */
            TRY(orion_rmw(d, REG_TX_GAIN_FORCE, i, 1, 1));
            TRY(orion_write(d, (uint16_t)(REG_TX0_FINAL_GAIN + i),
                            (uint8_t)(final_av & 0x1Fu)));
        }
    }

    switch (bias) {
    case HAL_BIAS_MAX:    lna_curr = 3; cmb = 3; drv = 31; break;
    case HAL_BIAS_LOW:    lna_curr = 0; cmb = 0; drv = 12; break;
    case HAL_BIAS_NOM:    lna_curr = 3; cmb = 1; drv = 31; break;
    case HAL_BIAS_2W_FEM: lna_curr = 0; cmb = 0; drv = 16; break;
    case HAL_BIAS_5W_FEM: lna_curr = 3; cmb = 1; drv = 26; break;
    default:              return ORION_ERR_ARG;
    }

    TRY(orion_rmw(d, REG_TX_LNA_CURR, 0, 2, lna_curr));
    TRY(set_ch_field(d, REG_TX_CMB_I0, ant_sel, 0, 2, cmb));
    TRY(set_ch_field(d, REG_TX_CMB_Q0, ant_sel, 0, 2, cmb));
    TRY(set_ch_field(d, REG_TX_CMB_I0, ant_sel, 2, 5, drv));   /* drv shares the reg */

    return ORION_OK;
}

/* -------------------------------------------------------------------------- */
orion_status_t hal_init_rx(orion_t *d, hal_bias_t bias, uint8_t ant_sel)
{
    uint8_t icur, qcur, gain;

    TRY(orion_write(d, REG_REG4_EXT_BIAS, 0x02));      /* v2 path */

    if (bias == HAL_BIAS_LOW) {
        /* v2: i=0, q=2 (the v1 path used q=0) */
        icur = 0; qcur = 2; gain = 8;
    } else {
        icur = 3; qcur = 3; gain = 15;
    }

    TRY(set_ch_field(d, REG_RX_CMB_I0, ant_sel, 0, 2, icur));
    TRY(set_ch_field(d, REG_RX_CMB_Q0, ant_sel, 0, 2, qcur));
    TRY(set_ch_field(d, REG_RX_CMB_I0, ant_sel, 2, 5, gain));

    return ORION_OK;
}

/* --------------------------------------------------------------------------
 * epsilon/tx_gain_phase_single_point_test.py
 * -------------------------------------------------------------------------- */
orion_status_t hal_tx_single_point(orion_t *d, uint8_t ant_sel,
                                   uint8_t p_idx, uint8_t g_idx,
                                   hal_bias_t bias, uint8_t final_av)
{
    uint8_t i;

    if (ant_sel == 0u || ant_sel > 0x0Fu) return ORION_ERR_ARG;

    TRY(hal_set_tr_mode(d, true));                 /* INT_TR   */
    TRY(hal_set_trx_mode(d, 1));                   /* TX       */
    TRY(hal_init_tx(d, bias, final_av, ant_sel));
    TRY(hal_set_tr_mask(d, ant_sel, -1));
    TRY(hal_cfg_stg2_load(d, true));               /* 'REG'    */
    TRY(hal_en_data_path(d, true));

    /* set_lut_idx(p_idx, g_idx, ant_sel) writes the phase and gain codes for
     * every selected channel. */
    for (i = 0; i < 4u; i++) {
        if (ant_sel & (1u << i)) {
            TRY(orion_set_phase_tx(d, i, p_idx));
            TRY(orion_set_gain_tx(d, i, g_idx));
        }
    }

    return hal_stg2_load(d);
}

/* --------------------------------------------------------------------------
 * epsilon/rx_gain_phase_single_point_test.py
 * -------------------------------------------------------------------------- */
orion_status_t hal_rx_single_point(orion_t *d, uint8_t ant_sel,
                                   uint8_t p_idx, uint8_t g_idx,
                                   bool dual_lut)
{
    uint8_t i;

    if (ant_sel == 0u || ant_sel > 0x0Fu) return ORION_ERR_ARG;

    TRY(hal_set_tr_mode(d, true));
    TRY(hal_set_trx_mode(d, 0));                   /* RX */
    TRY(hal_init_rx(d, HAL_BIAS_NOM, 0x0F));
    /*
     * The vendor script declares ant_sel at line 8 but then hardcodes 0xF at
     * lines 58 and 74, so every RX channel is enabled and written regardless.
     * We honour ant_sel instead, which is what the comment in the script
     * intends. Pass ant_sel = 0x0F to reproduce the vendor behaviour byte for
     * byte.
     */
    TRY(hal_set_tr_mask(d, -1, ant_sel));
    TRY(hal_set_freq(d, HAL_FREQ_9G));
    TRY(hal_cfg_stg2_load(d, true));
    TRY(hal_enable_rx_correction(d, true));
    TRY(hal_en_data_path(d, true));

    /*
     * The script's dual-LUT branch: gain indices above 31 move to the 11G bank
     * and LOW bias, everything else stays on 9G / NOM.
     */
    if (dual_lut && g_idx > 31u) {
        TRY(hal_set_freq(d, HAL_FREQ_11G));
        TRY(hal_init_rx(d, HAL_BIAS_LOW, ant_sel));
    } else {
        TRY(hal_set_freq(d, HAL_FREQ_9G));
        TRY(hal_init_rx(d, HAL_BIAS_NOM, ant_sel));
    }

    /* Write codes only to the selected channels. */
    for (i = 0; i < 4u; i++) {
        if (ant_sel & (1u << i)) {
            TRY(orion_set_phase_rx(d, i, p_idx));
            TRY(orion_set_gain_rx(d, i, g_idx));
        }
    }

    return hal_stg2_load(d);
}

/* --------------------------------------------------------------------------
 * epsilon/bfm_pa_bias_test.py
 * stage 0: program the DACs and assert TR + PA
 * stage 1: release TR so the bias visibly toggles on a multimeter
 * -------------------------------------------------------------------------- */
orion_status_t hal_pa_bias_test(orion_t *d, uint8_t tx_mask,
                                uint8_t on_code, uint8_t off_code, int stage)
{
    uint8_t i;

    if (stage != 0) {
        spi_hw_set_tr(false);                      /* spi.tr_reset() */
        return ORION_OK;
    }

    TRY(hal_set_tr_mask(d, tx_mask, -1));

    for (i = 0; i < 4u; i++)
        TRY(orion_write(d, (uint16_t)(REG_DAC_CTRL_PA0 + i), on_code & 0x7Fu));
    for (i = 0; i < 4u; i++)
        TRY(orion_write(d, (uint16_t)(REG_DAC_CTRL_PA0_PDN + i), off_code & 0x7Fu));

    spi_hw_set_tr(true);                           /* spi.tr_set() */
    spi_hw_set_pa(true);                           /* spi.pa_set() */

    return hal_en_data_path(d, true);
}

/* --------------------------------------------------------------------------
 * epsilon/bfm_lna_bias_test.py
 * -------------------------------------------------------------------------- */
orion_status_t hal_lna_bias_test(orion_t *d, uint8_t rx_mask,
                                 uint8_t on_code, uint8_t off_code, int stage)
{
    uint8_t i;

    if (stage != 0) {
        spi_hw_set_tr(false);
        return hal_en_data_path(d, true);          /* script re-writes TR_CFG */
    }

    TRY(hal_set_tr_mask(d, -1, rx_mask));

    /* Note the inversion relative to the PA test: LNA active code is 0
     * (-4.5 V) and the PDN code is 127 (-2.5 V). */
    for (i = 0; i < 4u; i++)
        TRY(orion_write(d, (uint16_t)(REG_DAC_CTRL_LNA0 + i), on_code & 0x7Fu));
    for (i = 0; i < 4u; i++)
        TRY(orion_write(d, (uint16_t)(REG_DAC_CTRL_LNA0_PDN + i), off_code & 0x7Fu));

    spi_hw_set_tr(true);
    return hal_en_data_path(d, true);
}

/* --------------------------------------------------------------------------
 * epsilon/sar_adc.py
 * -------------------------------------------------------------------------- */
/*
 * Programming guide 4.9 specifies the order explicitly:
 *   enable  : set en_adc, THEN set en_adc_osc
 *   disable : clear en_adc_osc, THEN clear en_adc
 *
 * The reference Python does the opposite on both. The datasheet order is
 * used here; it is the documented sequence and costs nothing.
 */
static orion_status_t adc_enable(orion_t *d, bool on)
{
    if (on) {
        TRY(orion_rmw(d, REG_ADC_CTRL, 1, 1, 1));      /* en_adc      */
        return orion_rmw(d, REG_ADC_CTRL, 0, 1, 1);    /* en_adc_osc  */
    }
    TRY(orion_rmw(d, REG_ADC_CTRL, 0, 1, 0));          /* en_adc_osc  */
    return orion_rmw(d, REG_ADC_CTRL, 1, 1, 0);        /* en_adc      */
}

/*
 * Poll ADC_STS.eoc for end of conversion (guide 4.9). Returns ORION_ERR_TIMEOUT
 * if the conversion never completes, instead of silently returning stale data
 * from a fixed delay.
 */
static orion_status_t adc_wait_eoc(orion_t *d)
{
    uint16_t tries;
    uint8_t sts = 0;

    for (tries = 0; tries < 200u; tries++) {
        TRY(orion_read(d, REG_ADC_STS, &sts));
        if (sts & 0x01u) return ORION_OK;
        spi_hw_delay_us(50);
    }
    return ORION_ERR_TIMEOUT;
}

static orion_status_t adc_sample(orion_t *d, uint16_t *out)
{
    uint8_t lsb = 0, msb = 0;
    TRY(orion_read(d, REG_ADC_IN_LSB, &lsb));
    TRY(orion_read(d, REG_ADC_IN_MSB, &msb));
    *out = (uint16_t)(((uint16_t)(msb & 0x01u) << 8) | lsb);   /* 9-bit, 0..511 */
    return ORION_OK;
}

orion_status_t hal_sar_adc_read(orion_t *d, uint8_t gp, uint16_t *out)
{
    if (out == 0 || gp < 4u || gp > 7u) return ORION_ERR_ARG;

    /* sel_adc_input(): clear every mux enable, then set gp7. Only GP7 has a
     * dedicated bit in REG0_ADC; gp4..gp6 share the same switch on this rev. */
    TRY(orion_write(d, REG_REG0_ADC, 0x00));
    TRY(orion_rmw(d, REG_REG0_ADC, 7, 1, 1));          /* en_gp7_to_adc_sw */

    /*
     * The script performs a throw-away conversion first and keeps only the
     * second reading. The first sample after enabling the oscillator is not
     * settled.
     */
    TRY(adc_enable(d, true));
    (void)adc_wait_eoc(d);
    { uint16_t discard; TRY(adc_sample(d, &discard)); }
    TRY(adc_enable(d, false));

    TRY(adc_enable(d, true));
    TRY(adc_wait_eoc(d));
    TRY(adc_sample(d, out));
    return adc_enable(d, false);
}

/* --------------------------------------------------------------------------
 * epsilon/bfm_DETx_test.py
 * -------------------------------------------------------------------------- */
static orion_status_t det_set_sel(orion_t *d, uint8_t ch, uint8_t sel)
{
    /* POWER_DET_CFG: 2 bits per detector, det0 at bits 1:0 */
    return orion_rmw(d, REG_POWER_DET_CFG, (uint8_t)(ch * 2u), 2, sel);
}

static orion_status_t det_sar(orion_t *d, uint16_t *v)
{
    /* get_sar_adc_value(): off, on, read, off - with settling between. */
    TRY(adc_enable(d, false));
    spi_hw_delay_us(200);
    TRY(adc_enable(d, true));
    TRY(adc_wait_eoc(d));
    TRY(adc_sample(d, v));
    return adc_enable(d, false);
}

orion_status_t hal_det_measure_rf_only(orion_t *d, uint8_t det_ch,
                                       int16_t *rf_only, uint8_t flash[4])
{
    uint16_t bias_only = 0, rf_plus_bias = 0;
    uint8_t r01 = 0, r23 = 0;

    if (det_ch > 3u || rf_only == 0) return ORION_ERR_ARG;

    /* Route the peak detector into the SAR ADC and enable all four detectors. */
    TRY(orion_rmw(d, REG_REG0_ADC, 4, 1, 1));          /* en_pkdet_to_adc_sw */
    TRY(orion_write(d, REG_TR_CTRL_2, 0xFF));          /* det_en_force + val  */
    TRY(orion_write(d, REG_POWER_DET_CFG, 0x00));      /* reset_all_det_sel() */

    /* sel = 2 -> bias only */
    TRY(det_set_sel(d, det_ch, 2));
    spi_hw_delay_us(1000);
    TRY(det_sar(d, &bias_only));

    /* sel = 1 -> RF + bias */
    TRY(det_set_sel(d, det_ch, 1));
    spi_hw_delay_us(1000);
    TRY(det_sar(d, &rf_plus_bias));

    *rf_only = (int16_t)((int16_t)rf_plus_bias - (int16_t)bias_only);

    /* Script toggles TR before sampling the flash ADCs. */
    spi_hw_set_tr(false);
    spi_hw_delay_us(500);
    spi_hw_set_tr(true);
    spi_hw_delay_us(500);

    if (flash != 0) {
        TRY(orion_read(d, REG_DET_0_1_ADC_OUT, &r01));
        TRY(orion_read(d, REG_DET_2_3_ADC_OUT, &r23));
        flash[0] = (uint8_t)(r01 & 0x07u);
        flash[1] = (uint8_t)((r01 & 0x38u) >> 3);
        flash[2] = (uint8_t)(r23 & 0x07u);
        flash[3] = (uint8_t)((r23 & 0x38u) >> 3);
    }

    return orion_write(d, REG_POWER_DET_CFG, 0x00);    /* reset_all_det_sel() */
}
