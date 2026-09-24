/* 
 * File:   LMX_8bit_regs.h
 * Author: HP
 *
 * Created on 18 September, 2026, 10:43 AM
 */

#ifndef LMX_8BIT_REGS_H
#define	LMX_8BIT_REGS_H

#include "main.h"

//#define UDIGCON0187

#define testbit(var,bit)    (var &= (1UL << bit))
#define getbit(var, bit)    (var = testbit(var,bit) >> bit)
#define setbit(var,bit)     (var |= (1UL << bit))      // To set a bit position
#define clearbit(var,bit)   (var &= (~(1UL << bit)))   // To clear a bit position
#define togglebit(var, bit) (var ^= (1UL << bit))      // To toggle a bit position


#define bits_to_transfer    24                         // For LMX device, the register is packed with parameter's address and value in it. So <23:16> for address and <15:0> for data.
#define MSB_MASK_24bit      0x800000                   // to mask the 23rd bit position of any value
#define bits_to_transfer_for_read        8             // while reading address value should be sent. this #define will be helpful there
#define bits_to_read        16                         // during the read operation, we need to monitor SDI pin status for 16 cycles

/*
 * In MCU_PLL_CTRL_V1 card, S16 and S17 pads are used for CSB pins of LMX1 and LMX2 respectively. 
 * 
 */
#define SCK_HIGH            (setbit(LATC, 3))           // setting serial clock functionality pin of MCU as high
#define SCK_LOW             (clearbit(LATC, 3))         // setting serial clock functionality pin of MCU as low
#define SDO_HIGH            (setbit(LATC, 5)) 			// setting serial data out functionality pin of MCU as high	
#define SDO_LOW             (clearbit(LATC, 5))			// setting serial data out functionality pin of MCU as low
// acts as first SPI slave device's (LMX1) CSB
#define LE_2_HIGH             (setbit(LATA, 4)) 		// setting chip select pin(GPIO) of MCU as high	
#define LE_2_LOW              (clearbit(LATA, 4))		// setting chip select pin(GPIO) of MCU as low		
#define LE_1_HIGH             (setbit(LATD, 2)) 		// setting chip select pin(GPIO) of MCU as high	
#define LE_1_LOW              (clearbit(LATD, 2))		// setting chip select pin(GPIO) of MCU as low	
#define PLL_regs_arr_size           113UL               // Number of PLL registers in LMX2594
#ifdef UDIGCON0187
#define PLL2_READ               (PORTGbits.RG15)
#define PLL1_READ               (PORTGbits.RG7)
#else
#define PLL1_READ               (PORTDbits.RD1)
#define PLL2_READ               (PORTDbits.RD0)
#endif
#define PLL1_ID                         0UL             // ID of LMX 1
#define PLL2_ID                         1UL             // ID of LMX 2

/*
 * Readily avaliable Bit shift operations
 */
#define BIT0 (1 << 0)
#define BIT1 (1 << 1)
#define BIT2 (1 << 2)
#define BIT3 (1 << 3)
#define BIT4 (1 << 4)
#define BIT5 (1 << 5)
#define BIT6 (1 << 6) 
#define BIT7 (1 << 7)
#define BIT8 (1 << 8)
#define BIT9 (1 << 9)
#define BIT10 (1 << 10)
#define BIT11 (1 << 11)

#define MIN_N               36UL                        // Minimum interger value 
#define REF_IN              100e3                       // 40MHz Reference input clock
#define PFD                 100e3                       // Phase detector frequency is set as 100MHz
#define F_VCO_MIN           7.5e6                       // LMX2595 device's min VCO frequency
#define F_VCO_MAX           15e6                        // LMX2595 device's max VCO frequency
#define fractional_denom    4294967296UL                // Considered this Fractional denominator value for calculations
#define CHANNELA            0                           // To get output from channel A of LMX2594. 
#define CHANNELB            1UL                         // To get output from channel B of LMX2594. 
#define CHANNEL_BOTH        2UL                         // To get output from both channels A & B of LMX2594
#define CHANNEL_NONE        3UL                         // To get no output from both channels A & B of LMX2594
#define MIN_FREQ            10.0e3                      // Minimum frequency that should be generated from the LMX device used in this ARMY WB project
#define MAX_FREQ            20.0e6                      // Maximum frequency that should be generated from the LMX device used in this ARMY WB project
#define N_DIV_RATIOS        18UL

#ifdef	__cplusplus
extern "C" {
#endif




#ifdef	__cplusplus
}
#endif

#endif	/* LMX_8BIT_REGS_H */

