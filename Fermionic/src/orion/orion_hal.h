/*
 * orion_hal.h - epsilon-variant bring-up sequences, ported to C
 * ---------------------------------------------------------------------------
 * Direct port of the orion_sdk epsilon test scripts, which FermionIC identify as the
 * vendor sequences for this board. Every register write below was verified
 * against a trace captured from the real ORION_8G_12G_hal Python class running
 * the epsilon scripts, so the chip sees byte-identical traffic.
 *
 * epsilon uses version = 'v2', so the v2 code paths in the Python HAL are the
 * ones reproduced here (notably REG4_EXT_BIAS.rsvd7 = 0x02, which the v1 path
 * does not write).
 *
 * Source scripts covered:
 *   sanity.py                            -> orion_probe() in orion.c
 *   tx_gain_phase_single_point_test.py   -> hal_tx_single_point()
 *   rx_gain_phase_single_point_test.py   -> hal_rx_single_point()
 *   bfm_pa_bias_test.py                  -> hal_pa_bias_test()
 *   bfm_lna_bias_test.py                 -> hal_lna_bias_test()
 *   sar_adc.py                           -> hal_sar_adc_read()
 *   bfm_DETx_test.py                     -> hal_det_measure_rf_only()
 */

#ifndef ORION_HAL_H
#define ORION_HAL_H

#include <stdint.h>
#include <stdbool.h>
#include "orion.h"

#ifdef __cplusplus
extern "C" {
#endif

/* --- register addresses used by the epsilon sequences -------------------- */
#define REG_UPDATE_CODE      0x012u
#define REG_RSVD0            0x013u   /* commits an UPDATE_CODE stage-2 load  */
#define REG_BEAM_CODE_R      0x014u
#define REG_RSVD1            0x015u   /* commits a BEAM_CODE load             */
#define REG_BEAM_CFG         0x016u
#define REG_STG2_CFG         0x017u
#define REG_FREQ_ID          0x018u
#define REG_RSVD2            0x019u
#define REG_RSVD3            0x01Au
#define REG_CORR_CFG         0x01Bu
#define REG_PAGE_ID          0x072u   /* selects the 256-byte LUT bank        */
#define REG_POWER_DET_CFG    0x073u
#define REG_DAC_CTRL_PA0     0x074u   /* PA0..PA3     = 0x074..0x077 */
#define REG_DAC_CTRL_PA0_PDN 0x078u   /* PA0..PA3 PDN = 0x078..0x07B */
#define REG_DAC_CTRL_LNA0    0x07Cu   /* LNA0..LNA3     = 0x07C..0x07F */
#define REG_DAC_CTRL_LNA0_PDN 0x080u  /* LNA0..LNA3 PDN = 0x080..0x083 */
#define REG_TX_CMB_I0        0x084u   /* stride 2 per channel */
#define REG_TX_CMB_Q0        0x086u
#define REG_RX_CMB_I0        0x08Cu   /* stride 2 per channel */
#define REG_RX_CMB_Q0        0x08Eu
#define REG_REG4_EXT_BIAS    0x098u
#define REG_REG0_BGR         0x09Bu
#define REG_REG0_ADC         0x09Cu
#define REG_ADC_CTRL         0x09Eu
#define REG_ADC_STS          0x09Fu   /* bit0 = eoc                           */
#define REG_TX_LNA_CURR      0x0A2u
#define REG_TR_CTRL_2        0x0A6u
#define REG_TR_MASK          0x0AAu
#define REG_TR_SW_CTRL       0x0ABu
#define REG_TR_CFG           0x0ACu
#define REG_TX_GAIN_FORCE    0x0B1u
#define REG_TX0_FINAL_GAIN   0x0B2u   /* TX0..TX3 = 0x0B2..0x0B5 */
#define REG_DET_0_1_ADC_OUT  0x0C2u
#define REG_DET_2_3_ADC_OUT  0x0C3u
#define REG_TEMP_CORR_CFG    0x0C5u
#define REG_TX_LNA_CFG       0x0C6u
#define REG_ADC_IN_LSB       0x0C9u
#define REG_ADC_IN_MSB       0x0CAu

/* --- bias modes, matching the Python strings ----------------------------- */
typedef enum {
    HAL_BIAS_MAX = 0,       /* 'MAX'    - TX Psat work            */
    HAL_BIAS_NOM,           /* 'NOM'    - nominal                 */
    HAL_BIAS_LOW,           /* 'LOW'    - backed off              */
    HAL_BIAS_2W_FEM,        /* '2W_FEM'                           */
    HAL_BIAS_5W_FEM         /* '5W_FEM'                           */
} hal_bias_t;

typedef enum { HAL_FREQ_9G = 0, HAL_FREQ_11G = 1 } hal_freq_t;

/* --- low-level building blocks (mirror the Python HAL methods) ----------- */
orion_status_t hal_set_tr_mode(orion_t *d, bool internal);   /* INT_TR / EXT_TR   */
orion_status_t hal_set_trx_mode(orion_t *d, uint8_t tx);     /* 1 = TX, 0 = RX    */
orion_status_t hal_set_tr_mask(orion_t *d, int tx_mask, int rx_mask); /* -1 = skip */
orion_status_t hal_cfg_stg2_load(orion_t *d, bool use_reg);
orion_status_t hal_en_data_path(orion_t *d, bool on);
orion_status_t hal_enable_rx_correction(orion_t *d, bool on);
orion_status_t hal_set_freq(orion_t *d, hal_freq_t f);
orion_status_t hal_init_tx(orion_t *d, hal_bias_t bias, uint8_t final_av, uint8_t ant_sel);
orion_status_t hal_init_rx(orion_t *d, hal_bias_t bias, uint8_t ant_sel);
orion_status_t hal_stg2_load(orion_t *d);

/* --- epsilon test sequences ---------------------------------------------- */

/*
 * epsilon/tx_gain_phase_single_point_test.py
 * Covers TX Psat, TX P1dB, TX gain change, TX phase change.
 * Use g_idx = 0 for the Psat / P1dB measurements.
 */
orion_status_t hal_tx_single_point(orion_t *d, uint8_t ant_sel,
                                   uint8_t p_idx, uint8_t g_idx,
                                   hal_bias_t bias, uint8_t final_av);

/*
 * epsilon/rx_gain_phase_single_point_test.py
 * Covers RX NF, RX gain change, RX phase change, RX P1dB.
 * dual_lut selects the bias/band split the script performs at g_idx > 31.
 */
orion_status_t hal_rx_single_point(orion_t *d, uint8_t ant_sel,
                                   uint8_t p_idx, uint8_t g_idx,
                                   bool dual_lut);

/* epsilon/bfm_pa_bias_test.py - stage 0 arms, stage 1 toggles TR */
orion_status_t hal_pa_bias_test(orion_t *d, uint8_t tx_mask,
                                uint8_t on_code, uint8_t off_code, int stage);

/* epsilon/bfm_lna_bias_test.py */
orion_status_t hal_lna_bias_test(orion_t *d, uint8_t rx_mask,
                                 uint8_t on_code, uint8_t off_code, int stage);

/* epsilon/sar_adc.py - input is 4..7 for gp4..gp7 */
orion_status_t hal_sar_adc_read(orion_t *d, uint8_t gp, uint16_t *out);

/* epsilon/bfm_DETx_test.py
 * Returns adc_rf_only = (RF+bias) - (bias only) for one detector channel,
 * plus the 3-bit flash ADC reading for all four detectors. */
orion_status_t hal_det_measure_rf_only(orion_t *d, uint8_t det_ch,
                                       int16_t *rf_only, uint8_t flash[4]);

#ifdef __cplusplus
}
#endif
#endif /* ORION_HAL_H */
