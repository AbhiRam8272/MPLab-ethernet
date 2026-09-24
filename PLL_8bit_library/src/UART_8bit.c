#include "main.h"
#include "UART_8bit.h"

/*
 * *
 * This function initializes UART1 module of MCU.
 *
 * @param	baud_rate holds the desired baud rate value.
 *
 * @note	None
 * */
void UART_Init(unsigned long baud_rate)
{            
    TXSTAbits.TXEN = 1;     // Enabling Transmitter
    TXSTAbits.SYNC = 0;     // Asychronous mode
    TXSTAbits.BRGH = 0;     // Low speed
    RCSTAbits.SPEN = 1;     // serial port enable
    RCSTAbits.CREN = 1;     // continuous receive enable
    BAUDCONbits.BRG16 = 0;  // 8-bit baud generator
    SPBRG = (unsigned char)(((_XTAL_FREQ/ baud_rate)/64U)-1);  // SPBRG = 64 -> 9600 baud rate when _XTAL_FREQ(FOSC/4) is 10MHz.   
}

/*
 * *
 * This function performs transferring of a byte/character to receiver end communicating over UART protocol.
 *
 * @param	cha expects a character to be sent
 *
 * @note	None
 * */
void UART_Write_Byte(char cha)
{   
    TXREG = cha;                  // Loading the character that need to be transmitted.
    while(!TXSTAbits.TRMT);       // Wait till the buffer is full.
}

/*
 * *
 * This function performs transferring of a string/array of characters to receiver end communicating over UART protocol.
 *
 * @param	str expects a string/text to be sent
 *
 * @note	None
 * */
void UART_Write_Text(char *str)
{
    while(*str)     UART_Write_Byte(*str++);
}
