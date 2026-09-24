/* 
 * File:   UART.h
 * Author: HP
 *
 * Created on 17 September, 2026, 6:52 PM
 */

#ifndef UART_H
#define	UART_H

#define BAUDRATE    9600    // UART baudrate
#define PACK1_LEN   8       // length for freqency packet (packet1)
#define PACK2_LEN   7       // length for power packet (packet2)
#define HDR1_BYTE   '#'     // header byte of packet1
#define FTR1_BYTE   '$'     // footer byte of packet1
#define HDR2_BYTE   '@'     // header byte of packet2
#define FTR2_BYTE   '*'     // footer byte of packet2

#ifdef	__cplusplus
extern "C" {
#endif

void UART_Init(unsigned long);
void UART_Write_Byte(char);
void UART_Write_Text(char *);


#ifdef	__cplusplus
}
#endif

#endif	/* UART_H */

