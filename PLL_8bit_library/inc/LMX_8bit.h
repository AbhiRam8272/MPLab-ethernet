/* 
 * File:   LMX_8bit.h
 * Author: HP
 *
 * Created on 18 September, 2026, 10:39 AM
 */

#ifndef LMX_8BIT_H
#define	LMX_8BIT_H

#include "LMX_8bit_regs.h"
#include "UART_8bit.h"
#include "SPI_8bit.h"

/*************************************************** Private variables begins *****************************************************************************/
_Bool once = 0;                         // Used this variable for limiting the response of start condition
_Bool comm_est_chk_flag, reg_data_flag, chan_pwr_data_flag;     // Used these variables as flags i.e., these flags are set whenever a desired packet structure is received
_Bool UART_buff_check_flag = 0;         // Used to variable to set whenever a character is received in UART interrupt
_Bool Down_conv_attn_packet_flag;       // Used this as flag when correct Dwon convertor attenuation packet is received.
_Bool Down_conv_bits[14];               // Exracting the bit position values of the array element and loading them into this boolean array
unsigned char  loop1 = 0;               // used in for loops
unsigned char recv_buff[15] = {'\0'};   // Character array to store received characters
unsigned char reg_addr = '\0';          // Used this variable to load register address retrieved from packet information.
unsigned char PLL_num = '\0';           // Used this variable to load PLL number(as we are using two PLLs in our application) retrieved from packet information.
unsigned char rx_data = '\0';           // A variable to store received character
unsigned int recv_buff_index = 0;       //  Used for array index position

unsigned int num_of_regs = 0;               // A variable used to store the number of ADF4159 registers which help in configuring the PLL.
unsigned int bit_indx = 0;                  // used in for loop for checking the each bit position of a 4 byte data
unsigned long val_to_write = 0;             // Used to load the array element value 

unsigned long reg44 = 0, reg45 = 0;         // Used for defining the output power need to be coming from, MUX state of OUTA and OUTB channels of LMX device etc.
unsigned int div_i = 0;                     // Used as divisor
long long int frac;                         // This variable is used for storing the fractional value after computation
unsigned long frac_lsw = 0, frac_msw = 0;   // These variables are for Least significant word and Most siginificant word of fractional value     
unsigned long int_n = 0;                    // Used for loading the integer value after frequency computation       
double n_step = PFD;                        // Declared and initialized with PFD value of 100MHz
double frac_step = 0;                       // Fractional step
double fre=0, f_vco=0;                      // Variables used for loading the frequency values that was derived from the packet structure
unsigned int div_ratios[N_DIV_RATIOS] = {2,4,6,8,12,16,24,32,48,64,72,96,128,192,256,384,512,768}; // Defined this LUT based on the info given LMX2594 device
unsigned short power_value = 31;            // If the power of the signal need to be changed then change the value here
unsigned long freq_offset = 0;              // used this variable to add to the extracted frequency word
unsigned long frequency, freq1, freq2, freq_for_DC_check = 0;    // Variables used for loading the frequency word value that was extracted from packet structure
unsigned long read_pll1_reg = 0, read_pll2_reg = 0;
/*************************************************** Private variables ends *****************************************************************************/

/* Below array contains all 113 PLL register address combined with data such that upon writing these registers to LMX device, it generates an output frequency. Detail info can be found in LMX2594 subject register.
In below array, element present in array index 85 location represents as VCO doubler. Changed the value from 0x1B0002 to 0x1B0000
*/
unsigned long Reg_write_vals_to_LMX[2][PLL_regs_arr_size] = {
    {0x700000,
0x6F0000,
0x6E0000,
0x6D0000,
0x6C0000,
0x6B0000,
0x6A0002,
0x691080,
0x680000,
0x670000,
0x660000,
0x650000,
0x64042B,
0x636F16,
0x62FECC,
0x610800,
0x600000,
0x5F0000,
0x5E0000,
0x5D0000,
0x5C0000,
0x5B0000,
0x5A0000,
0x590000,
0x580000,
0x570000,
0x560001,
0x550000,
0x540001,
0x53FFFF,
0x52FFFF,
0x510000,
0x500000,
0x4F0240,
0x4E0003,
0x4D0000,
0x4C000C,
0x4B0800,
0x4A0000,
0x49003F,
0x480001,
0x470041,
0x46C350,
0x450000,
0x4403E8,
0x430000,
0x4201F4,
0x410000,
0x401388,
0x3F0000,
0x3E0322,
0x3D00A8,
0x3C0000,
0x3B0001,
0x3A9001,
0x390020,
0x380000,
0x370000,
0x360000,
0x350000,
0x340820,
0x330080,
0x320000,
0x314180,
0x300300,
0x2F0300,
0x2E07FC,
0x2DC0F2,                                                          // max power from Channel A
0x2C3223,                                                           // Changed the value from 0x2C1F63 to 0x2C1F23 for using both Channel outputs.max power from Channel B
0x2B0000,
0x2A0000,
0x290000,
0x280000,
0x27FFFF,
0x26FFFF,
0x250004,
0x240000,
0x230004,
0x220000,
0x211E21,
0x200393,
0x1F03EC,
0x1E318C,
0x1D318C,
0x1C0488,
0x1B0000,
0x1A0DB0,
0x190C2B,
0x18071A,
0x17007C,
0x160001,
0x150401,
0x14E048,
0x1327B7,
0x120064,
0x11012C,
0x100080,
0x0F064F,
0x0E1E40,
0x0D4000,
0x0C5001,
0x0B0008,
0x0A10D8,
0x090604,
0x082000,
0x0700B2,
0x06C802,
0x0500C8,
0x040A43,
0x030642,
0x020500,
0x010808,
0x002418   },
{
0x700000,
0x6F0000,
0x6E0000,
0x6D0000,
0x6C0000,
0x6B0000,
0x6A0002,
0x691080,
0x680000,
0x670000,
0x660000,
0x650000,
0x64042B,
0x636F16,
0x62FECC,
0x610800,
0x600000,
0x5F0000,
0x5E0000,
0x5D0000,
0x5C0000,
0x5B0000,
0x5A0000,
0x590000,
0x580000,
0x570000,
0x560001,
0x550000,
0x540001,
0x53FFFF,
0x52FFFF,
0x510000,
0x500000,
0x4F0240,
0x4E0003,
0x4D0000,
0x4C000C,
0x4B0800,
0x4A0000,
0x49003F,
0x480001,
0x470041,
0x46C350,
0x450000,
0x4403E8,
0x430000,
0x4201F4,
0x410000,
0x401388,
0x3F0000,
0x3E0322,
0x3D00A8,
0x3C0000,
0x3B0001,
0x3A9001,
0x390020,
0x380000,
0x370000,
0x360000,
0x350000,
0x340820,
0x330080,
0x320000,
0x314180,
0x300300,
0x2F0300,
0x2E07FC,
0x2DC0F2,                                                          // max power from Channel A
0x2C3223,                                                           // Changed the value from 0x2C1F63 to 0x2C1F23 for using both Channel outputs.max power from Channel B
0x2B0000,
0x2A0000,
0x290000,
0x280000,
0x27FFFF,
0x26FFFF,
0x250004,
0x240000,
0x230004,
0x220000,
0x211E21,
0x200393,
0x1F03EC,
0x1E318C,
0x1D318C,
0x1C0488,
0x1B0000,
0x1A0DB0,
0x190C2B,
0x18071A,
0x17007C,
0x160001,
0x150401,
0x14E048,
0x1327B7,
0x120064,
0x11012C,
0x100080,
0x0F064F,
0x0E1E40,
0x0D4000,
0x0C5001,
0x0B0008,
0x0A10D8,
0x090604,
0x082000,
0x0700B2,
0x06C802,
0x0500C8,
0x040A43,
0x030642,
0x020500,
0x010808,
0x002418   
}
};

void LMX_WRITE(unsigned char, unsigned long);
unsigned long LMX_READ(unsigned int SPI_line, unsigned long read_address);
void Load_registers(unsigned int SPI_line, unsigned long array[2][PLL_regs_arr_size]);
void LMX_PLL_regs_write(unsigned char PLL_ID);
unsigned long calc_n(double f, double n_step,unsigned long div);
void LMX_set_chan_power(unsigned int PLL_ID, unsigned char channel, unsigned char power);
void LMX_set_freq(unsigned char, unsigned long);

#ifdef	__cplusplus
extern "C" {
#endif




#ifdef	__cplusplus
}
#endif

#endif	/* LMX_8BIT_H */

