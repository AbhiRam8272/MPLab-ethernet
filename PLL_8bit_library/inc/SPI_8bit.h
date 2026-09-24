/* 
 * File:   SPI.h
 * Author: HP
 *
 * Created on 17 September, 2026, 6:17 PM
 */

#ifndef SPI_H
#define	SPI_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <xc.h> // include processor files - each processor file is guarded.  


void SPI_Init(void);
void SPI_Write(unsigned char);
    
#ifdef	__cplusplus
}
#endif

#endif	/* SPI_H */

