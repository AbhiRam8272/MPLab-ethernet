/*
 * File:   main.c
 * Author: HP
 *
 * Created on 18 September, 2026, 10:19 AM
 *
 * Project: LMX2594 PLL Control (8-bit) using PIC18F4520
 *
 * Description:
 * This application controls two LMX2594 PLL devices through SPI by programming
 * required PLL registers (frequency, channel power) based on commands received
 * over UART. It supports two command types:
 * 1) Frequency programming packet (enables PLL output and sets target frequency).
 * 2) Channel power programming packet (updates output power level).
 *
 * UART:
 * - Receives encrypted command packets.
 * - Uses a receive buffer, packet validation, and flags to trigger processing in main loop.
 * - Sends ACK/NACK responses and basic status information to the host.
 * UART pin usage (PIC18F4520 typical USART1 mapping):
 * - TX  : RC6 / TX1
 * - RX  : RC7 / RX1
 *
 * SPI:
 * - Provides hardware SPI write capability to program LMX2594 8-bit frame format.
 * - Controls LE/CS lines for PLL1 and PLL2 to latch register updates.
 * * - SCK : RC3
 * - SDO (MOSI to LMX) : RC5
 * - (SS/CS): RD2  (controlled using LE_1/LE_2 macros for PLL1/PLL2)
 *
 * LMX control pins (from your macros):
 * - LE_1 : RD2
 * - LE_2 : RA4
 * 
 * LMX status/read pins:
 * - PLL1_READ : RD1 
 * - PLL2_READ : RD0 
 *
 * Target MCU:
 * - PIC18F4520
 *
 * Author/Team: <Your Name>
 * Date: <DD-MM-YYYY>
 */
// PIC18F4520 Configuration Bit Settings

// 'C' source line config statements

// CONFIG1H
#pragma config OSC = HS         // Oscillator Selection bits (HS oscillator)
#pragma config FCMEN = OFF      // Fail-Safe Clock Monitor Enable bit (Fail-Safe Clock Monitor disabled)
#pragma config IESO = OFF       // Internal/External Oscillator Switchover bit (Oscillator Switchover mode disabled)

// CONFIG2L
#pragma config PWRT = ON        // Power-up Timer Enable bit (PWRT enabled)
#pragma config BOREN = OFF      // Brown-out Reset Enable bits (Brown-out Reset disabled in hardware and software)
#pragma config BORV = 3         // Brown Out Reset Voltage bits (Minimum setting)

// CONFIG2H
#pragma config WDT = OFF        // Watchdog Timer Enable bit (WDT disabled (control is placed on the SWDTEN bit))
#pragma config WDTPS = 32768    // Watchdog Timer Postscale Select bits (1:32768)

// CONFIG3H
#pragma config CCP2MX = PORTC   // CCP2 MUX bit (CCP2 input/output is multiplexed with RC1)
#pragma config PBADEN = ON      // PORTB A/D Enable bit (PORTB<4:0> pins are configured as analog input channels on Reset)
#pragma config LPT1OSC = OFF    // Low-Power Timer1 Oscillator Enable bit (Timer1 configured for higher power operation)
#pragma config MCLRE = ON       // MCLR Pin Enable bit (MCLR pin enabled; RE3 input pin disabled)

// CONFIG4L
#pragma config STVREN = ON      // Stack Full/Underflow Reset Enable bit (Stack full/underflow will cause Reset)
#pragma config LVP = OFF        // Single-Supply ICSP Enable bit (Single-Supply ICSP disabled)
#pragma config XINST = OFF      // Extended Instruction Set Enable bit (Instruction set extension and Indexed Addressing mode disabled (Legacy mode))

// CONFIG5L
#pragma config CP0 = OFF        // Code Protection bit (Block 0 (000800-001FFFh) not code-protected)
#pragma config CP1 = OFF        // Code Protection bit (Block 1 (002000-003FFFh) not code-protected)
#pragma config CP2 = OFF        // Code Protection bit (Block 2 (004000-005FFFh) not code-protected)
#pragma config CP3 = OFF        // Code Protection bit (Block 3 (006000-007FFFh) not code-protected)

// CONFIG5H
#pragma config CPB = OFF        // Boot Block Code Protection bit (Boot block (000000-0007FFh) not code-protected)
#pragma config CPD = OFF        // Data EEPROM Code Protection bit (Data EEPROM not code-protected)

// CONFIG6L
#pragma config WRT0 = OFF       // Write Protection bit (Block 0 (000800-001FFFh) not write-protected)
#pragma config WRT1 = OFF       // Write Protection bit (Block 1 (002000-003FFFh) not write-protected)
#pragma config WRT2 = OFF       // Write Protection bit (Block 2 (004000-005FFFh) not write-protected)
#pragma config WRT3 = OFF       // Write Protection bit (Block 3 (006000-007FFFh) not write-protected)

// CONFIG6H
#pragma config WRTC = OFF       // Configuration Register Write Protection bit (Configuration registers (300000-3000FFh) not write-protected)
#pragma config WRTB = OFF       // Boot Block Write Protection bit (Boot block (000000-0007FFh) not write-protected)
#pragma config WRTD = OFF       // Data EEPROM Write Protection bit (Data EEPROM not write-protected)

// CONFIG7L
#pragma config EBTR0 = OFF      // Table Read Protection bit (Block 0 (000800-001FFFh) not protected from table reads executed in other blocks)
#pragma config EBTR1 = OFF      // Table Read Protection bit (Block 1 (002000-003FFFh) not protected from table reads executed in other blocks)
#pragma config EBTR2 = OFF      // Table Read Protection bit (Block 2 (004000-005FFFh) not protected from table reads executed in other blocks)
#pragma config EBTR3 = OFF      // Table Read Protection bit (Block 3 (006000-007FFFh) not protected from table reads executed in other blocks)

// CONFIG7H
#pragma config EBTRB = OFF      // Boot Block Table Read Protection bit (Boot block (000000-0007FFh) not protected from table reads executed in other blocks)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.
#define _XTAL_FREQ  10000000

#include <xc.h>
#include <stdio.h>
#include <pic18F4520.h>			// this header file contains the dsPIC30F5011 chip's essential data such as GPIO registers, UART registers etc.	

#include "LMX_8bit.h"

/*************************************************** User defined functions *****************************************************************************/
/*
 * *
 * This function performs configuration of ISR registers of UART1 module
 *
 * @param	None
 *
 * @note	None
 * */
void Config_interrupt(void)
{
    INTCONbits.GIE = 1;     // Global interrupt enabled
    INTCONbits.PEIE = 1;    // Peripheral interrupt enabled
    PIE1bits.RCIE = 1;      // UART receive interrupt enabled
    PIR1bits.RCIF = 0;      //UART receive interrupt flag disabled
    IPR1bits.RCIP = 1;      // UART receive interrupt priority high      
}

void Timer_Init(void)
{
	TMR2 = 0;				//Clear(or)reset Timer 2 register
	PR2 = 0X255;            //Load the period register for 1sec delay
	T2CONbits.T2CKPS0 = 1;	//Set prescaler value
	T2CONbits.T2CKPS1 = 1;	//as 16	
//	T2CONbits.TMR2ON = 1;	//Enable timer 2
//	PIE1bits.TMR2IE = 1;	//Enable Timer 2 interrupt
	PIR1bits.TMR2IF = 0;	//Clear interrupt flag
//	IPR1bits.TMR2IP = 5;	//Set interrupt priority level
}

/*
 * *
 * This function performs flushing the data present in the recv_buff array.
 *
 * @param	None
 *
 * @note	None
 * */
void flush(void)
{
    int index = 0;
    for(index = 0; index < sizeof(recv_buff); index++)  recv_buff[index] = '\0';
}

/*
 * *
 * This function performs processing of data present in the recv_buff array.
 * checks
 * @param	None
 *
 * @note	None
 * */
void process_recv_byte(void)
{
    if(recv_buff[0] != HDR1_BYTE && recv_buff[0] != HDR2_BYTE )    // expecting only these characters
        recv_buff_index = 0;                                       // if received character other than mentioned characters then clearing the array(where chars are stored) index value to 0   
    
    if(recv_buff_index == PACK1_LEN)                               // Checking if LMX frequency word packet condition is received or not
    {
        if(recv_buff[PACK1_LEN - 1] == FTR1_BYTE)
        {
            if(recv_buff[PACK1_LEN - 2] == FTR1_BYTE)
            {
                if(recv_buff[PACK1_LEN - PACK1_LEN] == HDR1_BYTE && (recv_buff[PACK1_LEN - PACK1_LEN + 1] == HDR1_BYTE))
                {
                    reg_data_flag = 1;
                    recv_buff_index = 0;                    
                }
            }            
        }         
    }
    if(recv_buff_index == PACK2_LEN)
    {
       if(recv_buff[PACK2_LEN - 1] == FTR2_BYTE)
       {
           if(recv_buff[PACK2_LEN - 2] == FTR2_BYTE)
           {
               if(recv_buff[PACK2_LEN - PACK2_LEN] == HDR2_BYTE && (recv_buff[PACK2_LEN - PACK2_LEN + 1] == HDR2_BYTE))
               {
                    chan_pwr_data_flag = 1;
                    recv_buff_index = 0;                   
               }
           }            
       }           
    } 
}


/*
 * *
 * This uart receive interrupt handler function which is called when any byte is received/present in the receive buffer
 *
 * @param	None
 *
 * @note	None
 * */
void __interrupt() ISR()
{
    if(PIR1bits.RCIF == 1)                              // Checking whether receive interrupt flag is enabled or not
    {       
        rx_data = RCREG;                                // Loading the received character into rx_data char variable
        recv_buff[recv_buff_index] = rx_data;           // Loading the content of rx_data char variable into a char array of size 10
        UART_Write_Byte(recv_buff[recv_buff_index]);
        recv_buff_index = recv_buff_index + 1;          // Incrementing the array index position        
        if(recv_buff_index == 15)
            recv_buff_index = 0;
        UART_buff_check_flag = 1; 
        PIR1bits.RCIF = 0;                              // Clearing the receive interrupt flag
    }
}

/************************************************************ MAIN FUNCTION STARTS ***********************************************************************/
int main(void) 
{                  
    ADCON1bits.PCFG = 0b1111;          //   Temporary configuration as ADC input channels are not required    

//    Initialized all pins state as logic low
    LATA = 0x00;
    LATB = 0x00;                  
    LATC = 0x00;
    LATD = 0x00;
    LATE = 0x00;
    
//    Configuring every avaliable IO PORT registers as output
    TRISA = 0x00;
    TRISB = 0x00;            
    TRISC = 0x80; 
    TRISD = 0x00;                     
         
    Timer_Init();
    
    UART_Init(BAUDRATE);      // Initializing UART1 module whose baud rate is 115200 bps, 8 bit data transfer, no parity, 1 stop bit. This module need to be connected to FPGA in order to receive LMX frequency word and IF input and down convertor attenuation.
    __delay_ms(1000);
    Config_interrupt();        // Configuring interrupt related settings 
    SPI_Init();                                           
    
    SDO_HIGH;
    SCK_HIGH;
    LE_1_HIGH;
    LE_2_HIGH;  
    
    __delay_ms(2000);
    
    LMX_PLL_regs_write(PLL1_ID);
            
    setbit(Reg_write_vals_to_LMX[PLL1_ID][R0], 0);                // Setting power-down bit as logic high     
    LMX_WRITE(PLL1_ID, Reg_write_vals_to_LMX[PLL1_ID][R0]);       // Writing power down register value to LMX1
    
    __delay_ms(1000);
                
    clearbit(Reg_write_vals_to_LMX[PLL1_ID][R0], 0);              // Setting normal operation bit as logic low     
    LMX_WRITE(PLL1_ID, Reg_write_vals_to_LMX[PLL1_ID][R0]);       // Writing power down register value to LMX1    
    
    // default frequency from the pll devices
    LMX_set_freq(PLL1_ID, 6000000);              
    LMX_set_chan_power(PLL1_ID, 0, 0x32);                         // maximum power
    
    while(1)                                                      // Forever loop
    {             
        if(UART_buff_check_flag)                                  // This flag is set whenever a byte is loaded in UART receive buffer
        {
            process_recv_byte();                                  // That byte undergoes to a process
            UART_buff_check_flag = 0;                             // Clearing the flag
        }
        
        if(reg_data_flag)                                         // When LMX frequency word packet is sent by user, this flag sets.
        {
            /* As the frequency word is encrypted in 4 bytes, decryption is done below */
            frequency = (unsigned long)recv_buff[5];
            frequency |= (((unsigned long)recv_buff[4]) << 8);
            frequency |= (((unsigned long)recv_buff[3]) << 16);
            frequency |= (((unsigned long)recv_buff[2]) << 24);                                                                                    // Out of range - no frequency will be generated.
            
            if(frequency >= MIN_FREQ && frequency <= MAX_FREQ)    // Checking if the given frequency word is in the specified limits or not
            {
                clearbit(Reg_write_vals_to_LMX[PLL1_ID][R0], 0);         // Setting power-down bit as logic high     
                clearbit(Reg_write_vals_to_LMX[PLL2_ID][R0], 0);         // Setting power-down bit as logic high     
                LMX_WRITE(PLL1_ID, Reg_write_vals_to_LMX[PLL1_ID][R0]);  // Writing power down register value to LMX1
                LMX_WRITE(PLL2_ID, Reg_write_vals_to_LMX[PLL2_ID][R0]);  // Writing power down register value to LMX2                
                
                freq1 = frequency; 
                freq2 = frequency + 50e3;                                // modified on 07-02-2024. Adding frequency offset of 1MHz to actual frequency word and tuning the PLL in order to minimal the spur effect generated from PLL.      

                LMX_set_freq(PLL1_ID, freq1);        
                LMX_set_chan_power(PLL1_ID, 0, 0x32);                    // maximum power

                LMX_set_freq(PLL2_ID, freq2);        
                LMX_set_chan_power(PLL2_ID, 0, 0x32);                    // maximum power                
                
                read_pll1_reg = LMX_READ(PLL1_ID, 0x6E);
                read_pll2_reg = LMX_READ(PLL2_ID, 0x6E);              
                UART_Write_Text("ACK1");                                 // Reply/acknowledgment to user
                UART_Write_Byte(((read_pll1_reg & 0x600) >> 9));
                UART_Write_Byte(((read_pll2_reg & 0x600) >> 9));                         
            }
            else
            {                
                setbit(Reg_write_vals_to_LMX[PLL1_ID][R0], 0);           // Setting power-down bit as logic high     
                setbit(Reg_write_vals_to_LMX[PLL2_ID][R0], 0);           // Setting power-down bit as logic high     
                LMX_WRITE(PLL1_ID, Reg_write_vals_to_LMX[PLL1_ID][R0]);  // Writing power down register value to LMX1
                LMX_WRITE(PLL2_ID, Reg_write_vals_to_LMX[PLL2_ID][R0]);  // Writing power down register value to LMX2
                read_pll1_reg = LMX_READ(PLL1_ID, 0x6E);
                read_pll2_reg = LMX_READ(PLL2_ID, 0x6E);                         
                UART_Write_Text("NACK");                                 // Reply/acknowledgment to user 
    
                UART_Write_Byte(((read_pll1_reg & 0x600) >> 9));
                UART_Write_Byte(((read_pll2_reg & 0x600) >> 9));         
            }              

            frequency = 0;
            freq1 = 0;
            freq2 = 0;
            flush();                                                     // Flushing the UART receive buffer.
            reg_data_flag = 0;                
        }
        
        // Added this logic on 24-05-2024. 
        if(chan_pwr_data_flag)                                           // if power change flag is set
        {
            LMX_set_chan_power( PLL1_ID, 0, recv_buff[4]);
            LMX_set_chan_power( PLL2_ID, 0, recv_buff[4]);
            UART_Write_Text("ACK2");
            flush(); 
            chan_pwr_data_flag = 0;
        }
    }    
    return 0;
}
/************************************************************ MAIN FUNCTION ENDS ***********************************************************************/