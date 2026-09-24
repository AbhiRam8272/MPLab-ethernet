#include "LMX_8bit.h"

/* This function is used to write the register value to either of the PLL. */
void LMX_WRITE(unsigned char SPI_line, unsigned long array_element)
{
    uint8_t dummy = 0;
    LE_1_HIGH;
    LE_2_HIGH;
    
    if(SPI_line == PLL1_ID)                     //  if SPI line is selected
        LE_1_LOW;                               // then PLL1 Latch enable is kept low
    else if(SPI_line == PLL2_ID)                // Else if SPI line 2 is selected
        LE_2_LOW;                               // then PLL 2 latch enable is kept low
    
    __delay_us(1);                              // delay of 2 micro-seconds 
    
    SPI_Write((array_element >> 16) & 0xFF);
    SPI_Write((array_element >> 8) & 0xFF);
    SPI_Write((array_element >> 0) & 0xFF);
    
    __delay_us(5);                              // delay of 2 micro-seconds   
    
    SCK_LOW;                                    // serial clock is kept low
    SDO_LOW;                                    //  serial data out is active low    
    
    if(SPI_line == PLL1_ID)                                                                           
        LE_1_HIGH;                              //  PLL1 Latch enable is kept high
    else if(SPI_line == PLL2_ID)
        LE_2_HIGH;                              // PLL2 Latch enable is kept high    
   
}

/* 
 * This function is used to read the register value of the PLL. 
 */
unsigned long LMX_READ(unsigned int SPI_line, unsigned long read_address)
{
    uint8_t loop1 = 0;                      // used in for loop
    unsigned long old_address = 0;          // to store the received address value
    unsigned long read_buffer = 0;          // to hold the little endian value
    unsigned long read_buffer_new = 0;      // to hold the big endian order value of read_buffer variable
    _Bool bit_val = 0;                      // this boolean gets set and clear based on MUXOUT pin status
    uint32_t shift_bit = 0;                 // used in bit by bit during little to big endian conversion
    SDO_HIGH;
    SCK_HIGH;     
    SSPCON1bits.SSPEN = 1;	//enable SPI           
    __delay_ms(100);                        // delay of 10 micro-seconds    
    if(SPI_line == 0)                       //  if SPI line is selected
        LE_1_LOW;                           // then PLL1 Latch enable is kept low
    else if(SPI_line == 1)                  // Else if SPI line 2 is selected
        LE_2_LOW;                           // then PLL 2 latch enable is kept low
  
    old_address = read_address;
    read_address |= 0x80;                                                       // during SPI read operation 7th bit should be set high, this is must 
    for(bit_indx = 0; bit_indx < bits_to_transfer_for_read; bit_indx++)         // iterating this loop for 8 times in order to send the address value
    {
        SCK_LOW;                            // serial clock is kept low

        if(read_address & 0x80)             // if the bit is logic 1 
        {
            SDO_HIGH;                       // then serial data out is active high
        }
        else                                // otherwise    
        {
            SDO_LOW;                        // active low
        }

        __delay_us(5);                      // delay of 8 micro-seconds
        SCK_HIGH;                           // serial clock is kept high
        __delay_us(5);                      // delay of 8 micro-seconds    

        read_address = read_address << 1;   // Left shifting the bit position
    }           
    for(bit_indx = 0; bit_indx < bits_to_read ; bit_indx++)                     // iterating this loop for 16 times in order to read the MUXOUT (SDI) pin status
    {
        SCK_LOW;                             // serial clock is kept low
        if(SPI_line == 0)
        {
            if(PLL1_READ)                    // if the status is logic 1 
            {
                bit_val = 1;                 // then this boolean is set high 
            }
            else                             // otherwise    
            {
                bit_val = 0;                 // low          
            }            
        }
        else
        {
            if(PLL2_READ)                    // if the status is logic 1 
            {
                bit_val = 1;                 // then this boolean is set high 
            }
            else                             // otherwise    
            {
                bit_val = 0;                 // low          
            }            
        }
        __delay_us(5);                       // delay of 8 micro-seconds
        SCK_HIGH;                            // serial clock is kept high
        __delay_us(5);                       // delay of 8 micro-seconds    

        read_buffer |= bit_val << bit_indx;  // Left shifting the bit position
    }  
    
    __delay_ms(5);                           // delay of 10 micro-seconds                
    
    SDO_HIGH;
    SCK_HIGH;     
    
    if(SPI_line == 0)                                                                           
        LE_1_HIGH;                           //  PLL1 Latch enable is kept high
    else if(SPI_line == 1)
        LE_2_HIGH;                           // PLL2 Latch enable is kept high             
   SSPCON1bits.SSPEN = 1;	//enable SPI
   __delay_ms(100);                          // delay of 10 micro-seconds 
   
    // shifting the value bit by bit and storing in a new variable
    for (loop1= 0; loop1 < 32; loop1++) {
        shift_bit = (read_buffer >> loop1) & 1;
        read_buffer_new |= shift_bit << (31 - loop1);
    }      
    
    return ((read_buffer_new) >> 16) | (old_address << 16);
}

void Load_registers(unsigned int SPI_line, unsigned long array[2][PLL_regs_arr_size])
{                 
        SCK_LOW;                                                        // serial clock is kept low
        SDO_LOW;                                                        //  serial data out is active low
        LE_1_HIGH;                                                      //  PLL1 Latch enable is kept high
        LE_2_HIGH;                                                      //  PLL2 Latch enable is kept high

        setbit(Reg_write_vals_to_LMX[SPI_line][112], 1);
        LMX_WRITE(SPI_line, Reg_write_vals_to_LMX[SPI_line][112]);
        clearbit(Reg_write_vals_to_LMX[SPI_line][112], 1);
        LMX_WRITE(SPI_line, Reg_write_vals_to_LMX[SPI_line][112]);
        
        __delay_us(5);                                                  // delay of 5 micro-seconds

        for(num_of_regs = 0; num_of_regs < PLL_regs_arr_size; num_of_regs++)    // Iterating the for loop for number of array elements in array times
        {
            val_to_write = Reg_write_vals_to_LMX[SPI_line][num_of_regs];        // Loading the array element value into a variable in order to keep the real data unaffected due to shifting operation done in ADF_WRITE function

            LMX_WRITE(SPI_line, val_to_write);                                  // Calling ADF_WRITE function to write the register value to the ADF4159 PLL
        }   

        LE_1_HIGH;                                                      //  PLL1 Latch enable is kept high
        LE_2_HIGH;                                                      //  PLL2 Latch enable is kept high
        SCK_LOW;                                                        // serial clock is kept low                                                                     
        SDO_LOW;                                                        //  serial data out is active low
}

void LMX_PLL_regs_write(unsigned char PLL_ID)
{    
    /* Loading registers into PLL1 and PLL 2 by which PLLs get configured to generate FMCW. */
    Load_registers(PLL_ID, Reg_write_vals_to_LMX);
}

unsigned long calc_n(double f, double n_step,unsigned long div)
{
    return f/(n_step/div);
}

/*
 * *
 * This function is vary the power of generated signal from the channels of PLLs
 *
 * @param	PLL_ID is ID of PLLs used. Since this MCU app can handle two PLLs (slave devices), for PLL1 the ID value is 1 and 2 for PLL2
 * @param	channel is holding channel A or channel B selection
 * @param  power is holding the desired power value
 * @note	None
 * */
void LMX_set_chan_power(unsigned int PLL_ID, unsigned char channel, unsigned char power)
{	
	if(channel==CHANNEL_BOTH)	//enable channel-A & channel-B
	{
		Reg_write_vals_to_LMX[PLL_ID][67] = 0x2DC0C0 | (power & 0x3F);  		//set channel-B power
		Reg_write_vals_to_LMX[PLL_ID][68] = 0x2C0023 | ((power & 0x3F) << 8);	//set channel-A power, careful while editing as MASH_ORDER and MASH_RESET_EN are also present here. We are using 3rd order delta-sigma modulator
	}

	else if(channel==CHANNELA)	//enable channel-A, disable channel-B
	{
		Reg_write_vals_to_LMX[PLL_ID][68] = 0x2C0023 | BIT7;					//set OUTB_PD
                   Reg_write_vals_to_LMX[PLL_ID][68] |= (power & 0x3F) << 8;    //set channel-A power
	}

	else if(channel==CHANNELB)	//enable channel-B, disable channel-A
	{
		Reg_write_vals_to_LMX[PLL_ID][68] = 0x2C0023 | BIT6;					//set OUTA_PD
                   Reg_write_vals_to_LMX[PLL_ID][67] = 0x2DC0C0 | (power & 0x3F);  //set channel-B power
	}

	else						//disable channels A and B
	{
		Reg_write_vals_to_LMX[PLL_ID][68] = 0x2C0023 | BIT7;                    //set OUTB_PD
		Reg_write_vals_to_LMX[PLL_ID][68] = 0x2C0023 | BIT6;                    //set OUTA_PD
	}
    
          LMX_WRITE(PLL_ID, Reg_write_vals_to_LMX[PLL_ID][68] );          
          LMX_WRITE(PLL_ID, Reg_write_vals_to_LMX[PLL_ID][67] );
} 

/*
 * *
 * This function is used to modify the necessary registers in order to generate the desired/asked frequency from the PLL
 *
 * @param	PLL_ID is ID of PLLs used. Since this MCU app can handle two PLLs (slave devices), for PLL1 the ID value is 1 and 2 for PLL2
 * @param	f is holding the asked frequency word value
 *
 * @note	None
 * */
void LMX_set_freq(unsigned char PLL_ID, unsigned long f)
{      
    f_vco = 0;
    frac = 0;
		
    fre=(double)f;
            
    n_step = PFD;
    Reg_write_vals_to_LMX[PLL_ID][101] = 0x0B0008 | ((int)(REF_IN/PFD) << 4);
//    LMX_WRITE(PLL_ID, Reg_write_vals_to_LMX[PLL_ID][101]);
     
    // Below if else logic was added on 05-04-2024. There were certain freq ranges for which the PFD_DLY value changes. This was handled in below if logic. 
    if((fre >= 1e6 && fre < 3e6) || (fre >= 4e6 && fre < 6e6) || (fre >= 7e6 && fre < 11e6) || (fre >= 16e6))
    {
       Reg_write_vals_to_LMX[PLL_ID][75] = 0x250304; 
//       LMX_WRITE(PLL_ID, Reg_write_vals_to_LMX[PLL_ID][75]);
    }
    else
    {
        Reg_write_vals_to_LMX[PLL_ID][75] = 0x250404; 
//        LMX_WRITE(PLL_ID, Reg_write_vals_to_LMX[PLL_ID][75]);
    }
     
    if(fre < F_VCO_MIN)   
    {		
        if(f>=3750e3)
            div_i=0;
        else if(f>=1875e3)
            div_i=1;    
        else if(f>=1250e3)
            div_i=2;
        else if(f>=938e3)
            div_i=3;
        else if(f>=625e3)
            div_i=4;
        else if(f>=469e3)
            div_i=5;
        else if(f>=313e3)
            div_i=6;
        else if(f>=235e3)
            div_i=7;
        else if(f>=157e3)
            div_i=8;
        else if(f>=118e3)
            div_i=9;
        else if(f>=79e3)
            div_i=11;
        else if(f>=59e3)
            div_i=12;
        else if(f>=40e3)
            div_i=13;
        else if(f>=30e3)
            div_i=14;
        else if(f>=20e3) 
            div_i=15;
        else if(f>=15e3)
            div_i=16;
        else if(f>=10e3)
            div_i=17;

        // Added below if else logic on 05-04-2024. As divisor value becomes greater than 2, the SEG1_EN should be enabled. Below if statement handles that
        if(div_i > 0){
            Reg_write_vals_to_LMX[PLL_ID][81] = 0x1F43EC;
//            LMX_WRITE(PLL_ID, Reg_write_vals_to_LMX[PLL_ID][81]);
        }
        else{
            Reg_write_vals_to_LMX[PLL_ID][81] = 0x1F03EC;
//            LMX_WRITE(PLL_ID, Reg_write_vals_to_LMX[PLL_ID][81]);
        }
        
        int_n = calc_n(f, n_step, div_ratios[div_i]);
        f_vco = f*div_ratios[div_i];

        frac = ((f_vco*fractional_denom)/n_step)-(int_n*fractional_denom);

        Reg_write_vals_to_LMX[PLL_ID][37] = 0x4B0800 | (div_i<<6);              // modified on 26-04-2024. previously here the register value was getting modified and getting updated in array. this caused freq generation issue while switching from >3750MHz to <3750MHz 
//        LMX_WRITE(PLL_ID, Reg_write_vals_to_LMX[PLL_ID][37]);

        //Reg_write_vals_to_LMX[][66] |= 0;                                     // channel divider = 0
        // clearing the 0th bit to enable the channel divider of Channel B
        clearbit(Reg_write_vals_to_LMX[PLL_ID][66],0);                          // modified on 12-07-2023. Previously, LMX device was able to generate to frequencies from 6.8 to 7.8GHz, but not from 7.499GHz to 6.8GHz. Because the muxout bit was getting toggled from 1 to 0 in this case.
//        LMX_WRITE(PLL_ID, Reg_write_vals_to_LMX[PLL_ID][66]);
        Reg_write_vals_to_LMX[PLL_ID][67] = 0x2DC0F2;                           // clearing the 11th bit to enable the vco of Channel A
//        LMX_WRITE(PLL_ID, Reg_write_vals_to_LMX[PLL_ID][67]);

        Reg_write_vals_to_LMX[PLL_ID][75] = 0x250004 | (4 << 8);        
//        LMX_WRITE(PLL_ID, Reg_write_vals_to_LMX[PLL_ID][75]);
        Reg_write_vals_to_LMX[PLL_ID][85] = 0x1B0002;                           // VCO doubler is disabled
//        LMX_WRITE(PLL_ID, Reg_write_vals_to_LMX[PLL_ID][85]);
    }	
    else              
    {                
        if(fre > F_VCO_MAX)
        {
            int_n = calc_n(f/2, n_step, 1);                                     // Dividing the given frequency value because of VCO doubler mode 
            f_vco = fre/2;
        }
        else
        {
            int_n = calc_n(f, n_step, 1);
            f_vco = fre;
        }
        
        frac = ((f_vco*fractional_denom)/n_step)-(int_n*fractional_denom);
        
        Reg_write_vals_to_LMX[PLL_ID][37] = 0x4B0800 | (0<<6);                  // ensuring that the channel divider value is set to 2 when frequencies are greater than 7.5GHz         
//        LMX_WRITE(PLL_ID, Reg_write_vals_to_LMX[PLL_ID][37]);
        
        if(fre > F_VCO_MAX)
        {
            
            Reg_write_vals_to_LMX[PLL_ID][67] = 0x2DD0F2;                       // setting the 12th bit to enable the vco doubler of Channel A for frequencies greater than 15GHz
//            LMX_WRITE(PLL_ID, Reg_write_vals_to_LMX[PLL_ID][67]);
            Reg_write_vals_to_LMX[PLL_ID][85] = 0x1B0003;                       // VCO doubler is enabled
//            LMX_WRITE(PLL_ID, Reg_write_vals_to_LMX[PLL_ID][85]);
        }
        else
        {
            setbit(Reg_write_vals_to_LMX[PLL_ID][66],0);                        // setting the 0th bit to enable the vco of Channel B
//            LMX_WRITE(PLL_ID, Reg_write_vals_to_LMX[PLL_ID][66]);
            Reg_write_vals_to_LMX[PLL_ID][67] = 0x2DC8F2;                       // setting the 11th bit to enable the vco of Channel A
//            LMX_WRITE(PLL_ID, Reg_write_vals_to_LMX[PLL_ID][67]);
            Reg_write_vals_to_LMX[PLL_ID][85] = 0x1B0002;                       // VCO doubler is disabled
//            LMX_WRITE(PLL_ID, Reg_write_vals_to_LMX[PLL_ID][85]);
        }
    }
    
//    frac = frac + 10000;
    frac = frac + 0;
    frac_lsw = (unsigned long)frac & 0xFFFF;
    frac_msw = ((unsigned long)frac >> 16) & 0xFFFF;	

    Reg_write_vals_to_LMX[PLL_ID][76] = (0x240000 | int_n);                     // Framing INT value
//   LMX_WRITE(PLL_ID, Reg_write_vals_to_LMX[PLL_ID][76]);
   
    Reg_write_vals_to_LMX[PLL_ID][70] = 0x2A0000 | frac_msw;                    // Framing fractional value at most significant word
//    LMX_WRITE(PLL_ID, Reg_write_vals_to_LMX[PLL_ID][70]);
    
    Reg_write_vals_to_LMX[PLL_ID][69] = 0x2B0000 | frac_lsw;                    // Framing fractional value at least significant word
//    LMX_WRITE(PLL_ID, Reg_write_vals_to_LMX[PLL_ID][69]);
    
    setbit(Reg_write_vals_to_LMX[PLL_ID][112], 3);
    
//    TMR1 = 0;                                       // reset timer counter register
//    T1CONbits.TON = 1;                      // enable timer        
    LMX_WRITE(PLL_ID, Reg_write_vals_to_LMX[PLL_ID][101]);
    LMX_WRITE(PLL_ID, Reg_write_vals_to_LMX[PLL_ID][75]);
    LMX_WRITE(PLL_ID, Reg_write_vals_to_LMX[PLL_ID][81]);
    LMX_WRITE(PLL_ID, Reg_write_vals_to_LMX[PLL_ID][37]);
    LMX_WRITE(PLL_ID, Reg_write_vals_to_LMX[PLL_ID][66]);
    LMX_WRITE(PLL_ID, Reg_write_vals_to_LMX[PLL_ID][67]);
    LMX_WRITE(PLL_ID, Reg_write_vals_to_LMX[PLL_ID][85]);
    LMX_WRITE(PLL_ID, Reg_write_vals_to_LMX[PLL_ID][76]);
    LMX_WRITE(PLL_ID, Reg_write_vals_to_LMX[PLL_ID][70]);
    LMX_WRITE(PLL_ID, Reg_write_vals_to_LMX[PLL_ID][69]);        
    LMX_WRITE(PLL_ID, Reg_write_vals_to_LMX[PLL_ID][112]);     
//    T1CONbits.TON = 0;                      // disable timer
    frac = 0;
}


