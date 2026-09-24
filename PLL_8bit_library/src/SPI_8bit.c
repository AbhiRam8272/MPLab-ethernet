#include "SPI_8bit.h"

void SPI_Init()
{
    SSPCON1bits.SSPEN = 1;      //enable SPI
    
    SSPSTATbits.SMP = 0;		//data sample at middle
    SSPSTATbits.CKE = 1;        // transmit to idle to active
    
    SSPCON1bits.CKP = 0;		//clock idle low
    SSPCON1bits.SSPM = 0b0000;  // Fosc/4 = 2.5MHz
}

void SPI_Write(unsigned char data)
{
    unsigned char dummy = 0;

    SSPBUF = data;
    while(!PIR1bits.SSPIF);
    PIR1bits.SSPIF = 0;
    dummy = SSPBUF;

}

