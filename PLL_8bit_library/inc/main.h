/* 
 * File:   main.h
 * Author: HP
 *
 * Created on 17 September, 2026, 6:57 PM
 */

#ifndef MAIN_H
#define	MAIN_H

#define _XTAL_FREQ  10000000
#define R0  112   // this register for powered down, normal operation, register reset, MUXout pin readback and lock detect, and mute output

#ifdef	__cplusplus
extern "C" {
#endif

#include <xc.h> // include processor files - each processor file is guarded.  
#include <stdio.h>
#include <pic18F4520.h>				// this header file contains the dsPIC30F5011 chip's essential data such as GPIO registers, UART registers etc.	


#ifdef	__cplusplus
}
#endif

#endif	/* MAIN_H */

