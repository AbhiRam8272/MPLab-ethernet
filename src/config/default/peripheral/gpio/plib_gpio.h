/*******************************************************************************
  GPIO PLIB

  Company:
    Microchip Technology Inc.

  File Name:
    plib_gpio.h

  Summary:
    GPIO PLIB Header File

  Description:
    This library provides an interface to control and interact with Parallel
    Input/Output controller (GPIO) module.

*******************************************************************************/

/*******************************************************************************
* Copyright (C) 2019 Microchip Technology Inc. and its subsidiaries.
*
* Subject to your compliance with these terms, you may use Microchip software
* and any derivatives exclusively with Microchip products. It is your
* responsibility to comply with third party license terms applicable to your
* use of third party software (including open source software) that may
* accompany Microchip software.
*
* THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
* EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
* WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
* PARTICULAR PURPOSE.
*
* IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
* INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
* WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
* BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
* FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN
* ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
* THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
*******************************************************************************/

#ifndef PLIB_GPIO_H
#define PLIB_GPIO_H

#include <device.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// DOM-IGNORE-BEGIN
#ifdef __cplusplus  // Provide C++ Compatibility

    extern "C" {

#endif
// DOM-IGNORE-END

// *****************************************************************************
// *****************************************************************************
// Section: Data types and constants
// *****************************************************************************
// *****************************************************************************


/*** Macros for PS8_6 pin ***/
#define PS8_6_Set()               (LATESET = (1<<5))
#define PS8_6_Clear()             (LATECLR = (1<<5))
#define PS8_6_Toggle()            (LATEINV= (1<<5))
#define PS8_6_OutputEnable()      (TRISECLR = (1<<5))
#define PS8_6_InputEnable()       (TRISESET = (1<<5))
#define PS8_6_Get()               ((PORTE >> 5) & 0x1)
#define PS8_6_PIN                  GPIO_PIN_RE5

/*** Macros for PS8_5 pin ***/
#define PS8_5_Set()               (LATESET = (1<<6))
#define PS8_5_Clear()             (LATECLR = (1<<6))
#define PS8_5_Toggle()            (LATEINV= (1<<6))
#define PS8_5_OutputEnable()      (TRISECLR = (1<<6))
#define PS8_5_InputEnable()       (TRISESET = (1<<6))
#define PS8_5_Get()               ((PORTE >> 6) & 0x1)
#define PS8_5_PIN                  GPIO_PIN_RE6

/*** Macros for PS8_4 pin ***/
#define PS8_4_Set()               (LATESET = (1<<7))
#define PS8_4_Clear()             (LATECLR = (1<<7))
#define PS8_4_Toggle()            (LATEINV= (1<<7))
#define PS8_4_OutputEnable()      (TRISECLR = (1<<7))
#define PS8_4_InputEnable()       (TRISESET = (1<<7))
#define PS8_4_Get()               ((PORTE >> 7) & 0x1)
#define PS8_4_PIN                  GPIO_PIN_RE7

/*** Macros for PS8_3 pin ***/
#define PS8_3_Set()               (LATCSET = (1<<1))
#define PS8_3_Clear()             (LATCCLR = (1<<1))
#define PS8_3_Toggle()            (LATCINV= (1<<1))
#define PS8_3_OutputEnable()      (TRISCCLR = (1<<1))
#define PS8_3_InputEnable()       (TRISCSET = (1<<1))
#define PS8_3_Get()               ((PORTC >> 1) & 0x1)
#define PS8_3_PIN                  GPIO_PIN_RC1

/*** Macros for PS8_2 pin ***/
#define PS8_2_Set()               (LATCSET = (1<<2))
#define PS8_2_Clear()             (LATCCLR = (1<<2))
#define PS8_2_Toggle()            (LATCINV= (1<<2))
#define PS8_2_OutputEnable()      (TRISCCLR = (1<<2))
#define PS8_2_InputEnable()       (TRISCSET = (1<<2))
#define PS8_2_Get()               ((PORTC >> 2) & 0x1)
#define PS8_2_PIN                  GPIO_PIN_RC2

/*** Macros for PS8_1 pin ***/
#define PS8_1_Set()               (LATCSET = (1<<3))
#define PS8_1_Clear()             (LATCCLR = (1<<3))
#define PS8_1_Toggle()            (LATCINV= (1<<3))
#define PS8_1_OutputEnable()      (TRISCCLR = (1<<3))
#define PS8_1_InputEnable()       (TRISCSET = (1<<3))
#define PS8_1_Get()               ((PORTC >> 3) & 0x1)
#define PS8_1_PIN                  GPIO_PIN_RC3

/*** Macros for PS7_6 pin ***/
#define PS7_6_Set()               (LATCSET = (1<<4))
#define PS7_6_Clear()             (LATCCLR = (1<<4))
#define PS7_6_Toggle()            (LATCINV= (1<<4))
#define PS7_6_OutputEnable()      (TRISCCLR = (1<<4))
#define PS7_6_InputEnable()       (TRISCSET = (1<<4))
#define PS7_6_Get()               ((PORTC >> 4) & 0x1)
#define PS7_6_PIN                  GPIO_PIN_RC4

/*** Macros for PS7_5 pin ***/
#define PS7_5_Set()               (LATGSET = (1<<6))
#define PS7_5_Clear()             (LATGCLR = (1<<6))
#define PS7_5_Toggle()            (LATGINV= (1<<6))
#define PS7_5_OutputEnable()      (TRISGCLR = (1<<6))
#define PS7_5_InputEnable()       (TRISGSET = (1<<6))
#define PS7_5_Get()               ((PORTG >> 6) & 0x1)
#define PS7_5_PIN                  GPIO_PIN_RG6

/*** Macros for PS7_4 pin ***/
#define PS7_4_Set()               (LATGSET = (1<<7))
#define PS7_4_Clear()             (LATGCLR = (1<<7))
#define PS7_4_Toggle()            (LATGINV= (1<<7))
#define PS7_4_OutputEnable()      (TRISGCLR = (1<<7))
#define PS7_4_InputEnable()       (TRISGSET = (1<<7))
#define PS7_4_Get()               ((PORTG >> 7) & 0x1)
#define PS7_4_PIN                  GPIO_PIN_RG7

/*** Macros for PS7_3 pin ***/
#define PS7_3_Set()               (LATASET = (1<<0))
#define PS7_3_Clear()             (LATACLR = (1<<0))
#define PS7_3_Toggle()            (LATAINV= (1<<0))
#define PS7_3_OutputEnable()      (TRISACLR = (1<<0))
#define PS7_3_InputEnable()       (TRISASET = (1<<0))
#define PS7_3_Get()               ((PORTA >> 0) & 0x1)
#define PS7_3_PIN                  GPIO_PIN_RA0

/*** Macros for PS7_2 pin ***/
#define PS7_2_Set()               (LATBSET = (1<<5))
#define PS7_2_Clear()             (LATBCLR = (1<<5))
#define PS7_2_Toggle()            (LATBINV= (1<<5))
#define PS7_2_OutputEnable()      (TRISBCLR = (1<<5))
#define PS7_2_InputEnable()       (TRISBSET = (1<<5))
#define PS7_2_Get()               ((PORTB >> 5) & 0x1)
#define PS7_2_PIN                  GPIO_PIN_RB5

/*** Macros for PS7_1 pin ***/
#define PS7_1_Set()               (LATBSET = (1<<4))
#define PS7_1_Clear()             (LATBCLR = (1<<4))
#define PS7_1_Toggle()            (LATBINV= (1<<4))
#define PS7_1_OutputEnable()      (TRISBCLR = (1<<4))
#define PS7_1_InputEnable()       (TRISBSET = (1<<4))
#define PS7_1_Get()               ((PORTB >> 4) & 0x1)
#define PS7_1_PIN                  GPIO_PIN_RB4

/*** Macros for EN_8 pin ***/
#define EN_8_Set()               (LATBSET = (1<<3))
#define EN_8_Clear()             (LATBCLR = (1<<3))
#define EN_8_Toggle()            (LATBINV= (1<<3))
#define EN_8_OutputEnable()      (TRISBCLR = (1<<3))
#define EN_8_InputEnable()       (TRISBSET = (1<<3))
#define EN_8_Get()               ((PORTB >> 3) & 0x1)
#define EN_8_PIN                  GPIO_PIN_RB3

/*** Macros for SW_9 pin ***/
#define SW_9_Set()               (LATBSET = (1<<2))
#define SW_9_Clear()             (LATBCLR = (1<<2))
#define SW_9_Toggle()            (LATBINV= (1<<2))
#define SW_9_OutputEnable()      (TRISBCLR = (1<<2))
#define SW_9_InputEnable()       (TRISBSET = (1<<2))
#define SW_9_Get()               ((PORTB >> 2) & 0x1)
#define SW_9_PIN                  GPIO_PIN_RB2

/*** Macros for GPIO_RB1 pin ***/
#define GPIO_RB1_Set()               (LATBSET = (1<<1))
#define GPIO_RB1_Clear()             (LATBCLR = (1<<1))
#define GPIO_RB1_Toggle()            (LATBINV= (1<<1))
#define GPIO_RB1_OutputEnable()      (TRISBCLR = (1<<1))
#define GPIO_RB1_InputEnable()       (TRISBSET = (1<<1))
#define GPIO_RB1_Get()               ((PORTB >> 1) & 0x1)
#define GPIO_RB1_PIN                  GPIO_PIN_RB1

/*** Macros for GPIO_RB0 pin ***/
#define GPIO_RB0_Set()               (LATBSET = (1<<0))
#define GPIO_RB0_Clear()             (LATBCLR = (1<<0))
#define GPIO_RB0_Toggle()            (LATBINV= (1<<0))
#define GPIO_RB0_OutputEnable()      (TRISBCLR = (1<<0))
#define GPIO_RB0_InputEnable()       (TRISBSET = (1<<0))
#define GPIO_RB0_Get()               ((PORTB >> 0) & 0x1)
#define GPIO_RB0_PIN                  GPIO_PIN_RB0

/*** Macros for SW_8 pin ***/
#define SW_8_Set()               (LATBSET = (1<<6))
#define SW_8_Clear()             (LATBCLR = (1<<6))
#define SW_8_Toggle()            (LATBINV= (1<<6))
#define SW_8_OutputEnable()      (TRISBCLR = (1<<6))
#define SW_8_InputEnable()       (TRISBSET = (1<<6))
#define SW_8_Get()               ((PORTB >> 6) & 0x1)
#define SW_8_PIN                  GPIO_PIN_RB6

/*** Macros for SW_7 pin ***/
#define SW_7_Set()               (LATBSET = (1<<7))
#define SW_7_Clear()             (LATBCLR = (1<<7))
#define SW_7_Toggle()            (LATBINV= (1<<7))
#define SW_7_OutputEnable()      (TRISBCLR = (1<<7))
#define SW_7_InputEnable()       (TRISBSET = (1<<7))
#define SW_7_Get()               ((PORTB >> 7) & 0x1)
#define SW_7_PIN                  GPIO_PIN_RB7

/*** Macros for SW_5 pin ***/
#define SW_5_Set()               (LATASET = (1<<9))
#define SW_5_Clear()             (LATACLR = (1<<9))
#define SW_5_Toggle()            (LATAINV= (1<<9))
#define SW_5_OutputEnable()      (TRISACLR = (1<<9))
#define SW_5_InputEnable()       (TRISASET = (1<<9))
#define SW_5_Get()               ((PORTA >> 9) & 0x1)
#define SW_5_PIN                  GPIO_PIN_RA9

/*** Macros for SW_6 pin ***/
#define SW_6_Set()               (LATASET = (1<<10))
#define SW_6_Clear()             (LATACLR = (1<<10))
#define SW_6_Toggle()            (LATAINV= (1<<10))
#define SW_6_OutputEnable()      (TRISACLR = (1<<10))
#define SW_6_InputEnable()       (TRISASET = (1<<10))
#define SW_6_Get()               ((PORTA >> 10) & 0x1)
#define SW_6_PIN                  GPIO_PIN_RA10

/*** Macros for EN_5 pin ***/
#define EN_5_Set()               (LATBSET = (1<<8))
#define EN_5_Clear()             (LATBCLR = (1<<8))
#define EN_5_Toggle()            (LATBINV= (1<<8))
#define EN_5_OutputEnable()      (TRISBCLR = (1<<8))
#define EN_5_InputEnable()       (TRISBSET = (1<<8))
#define EN_5_Get()               ((PORTB >> 8) & 0x1)
#define EN_5_PIN                  GPIO_PIN_RB8

/*** Macros for EN_7 pin ***/
#define EN_7_Set()               (LATBSET = (1<<9))
#define EN_7_Clear()             (LATBCLR = (1<<9))
#define EN_7_Toggle()            (LATBINV= (1<<9))
#define EN_7_OutputEnable()      (TRISBCLR = (1<<9))
#define EN_7_InputEnable()       (TRISBSET = (1<<9))
#define EN_7_Get()               ((PORTB >> 9) & 0x1)
#define EN_7_PIN                  GPIO_PIN_RB9

/*** Macros for EN_6 pin ***/
#define EN_6_Set()               (LATBSET = (1<<10))
#define EN_6_Clear()             (LATBCLR = (1<<10))
#define EN_6_Toggle()            (LATBINV= (1<<10))
#define EN_6_OutputEnable()      (TRISBCLR = (1<<10))
#define EN_6_InputEnable()       (TRISBSET = (1<<10))
#define EN_6_Get()               ((PORTB >> 10) & 0x1)
#define EN_6_PIN                  GPIO_PIN_RB10

/*** Macros for PS6_6 pin ***/
#define PS6_6_Set()               (LATBSET = (1<<11))
#define PS6_6_Clear()             (LATBCLR = (1<<11))
#define PS6_6_Toggle()            (LATBINV= (1<<11))
#define PS6_6_OutputEnable()      (TRISBCLR = (1<<11))
#define PS6_6_InputEnable()       (TRISBSET = (1<<11))
#define PS6_6_Get()               ((PORTB >> 11) & 0x1)
#define PS6_6_PIN                  GPIO_PIN_RB11

/*** Macros for PS6_5 pin ***/
#define PS6_5_Set()               (LATASET = (1<<1))
#define PS6_5_Clear()             (LATACLR = (1<<1))
#define PS6_5_Toggle()            (LATAINV= (1<<1))
#define PS6_5_OutputEnable()      (TRISACLR = (1<<1))
#define PS6_5_InputEnable()       (TRISASET = (1<<1))
#define PS6_5_Get()               ((PORTA >> 1) & 0x1)
#define PS6_5_PIN                  GPIO_PIN_RA1

/*** Macros for PS6_4 pin ***/
#define PS6_4_Set()               (LATFSET = (1<<13))
#define PS6_4_Clear()             (LATFCLR = (1<<13))
#define PS6_4_Toggle()            (LATFINV= (1<<13))
#define PS6_4_OutputEnable()      (TRISFCLR = (1<<13))
#define PS6_4_InputEnable()       (TRISFSET = (1<<13))
#define PS6_4_Get()               ((PORTF >> 13) & 0x1)
#define PS6_4_PIN                  GPIO_PIN_RF13

/*** Macros for PS6_3 pin ***/
#define PS6_3_Set()               (LATFSET = (1<<12))
#define PS6_3_Clear()             (LATFCLR = (1<<12))
#define PS6_3_Toggle()            (LATFINV= (1<<12))
#define PS6_3_OutputEnable()      (TRISFCLR = (1<<12))
#define PS6_3_InputEnable()       (TRISFSET = (1<<12))
#define PS6_3_Get()               ((PORTF >> 12) & 0x1)
#define PS6_3_PIN                  GPIO_PIN_RF12

/*** Macros for PS6_2 pin ***/
#define PS6_2_Set()               (LATBSET = (1<<12))
#define PS6_2_Clear()             (LATBCLR = (1<<12))
#define PS6_2_Toggle()            (LATBINV= (1<<12))
#define PS6_2_OutputEnable()      (TRISBCLR = (1<<12))
#define PS6_2_InputEnable()       (TRISBSET = (1<<12))
#define PS6_2_Get()               ((PORTB >> 12) & 0x1)
#define PS6_2_PIN                  GPIO_PIN_RB12

/*** Macros for PS6_1 pin ***/
#define PS6_1_Set()               (LATBSET = (1<<13))
#define PS6_1_Clear()             (LATBCLR = (1<<13))
#define PS6_1_Toggle()            (LATBINV= (1<<13))
#define PS6_1_OutputEnable()      (TRISBCLR = (1<<13))
#define PS6_1_InputEnable()       (TRISBSET = (1<<13))
#define PS6_1_Get()               ((PORTB >> 13) & 0x1)
#define PS6_1_PIN                  GPIO_PIN_RB13

/*** Macros for PS5_6 pin ***/
#define PS5_6_Set()               (LATBSET = (1<<14))
#define PS5_6_Clear()             (LATBCLR = (1<<14))
#define PS5_6_Toggle()            (LATBINV= (1<<14))
#define PS5_6_OutputEnable()      (TRISBCLR = (1<<14))
#define PS5_6_InputEnable()       (TRISBSET = (1<<14))
#define PS5_6_Get()               ((PORTB >> 14) & 0x1)
#define PS5_6_PIN                  GPIO_PIN_RB14

/*** Macros for PS5_1 pin ***/
#define PS5_1_Set()               (LATBSET = (1<<15))
#define PS5_1_Clear()             (LATBCLR = (1<<15))
#define PS5_1_Toggle()            (LATBINV= (1<<15))
#define PS5_1_OutputEnable()      (TRISBCLR = (1<<15))
#define PS5_1_InputEnable()       (TRISBSET = (1<<15))
#define PS5_1_Get()               ((PORTB >> 15) & 0x1)
#define PS5_1_PIN                  GPIO_PIN_RB15

/*** Macros for PS5_5 pin ***/
#define PS5_5_Set()               (LATFSET = (1<<4))
#define PS5_5_Clear()             (LATFCLR = (1<<4))
#define PS5_5_Toggle()            (LATFINV= (1<<4))
#define PS5_5_OutputEnable()      (TRISFCLR = (1<<4))
#define PS5_5_InputEnable()       (TRISFSET = (1<<4))
#define PS5_5_Get()               ((PORTF >> 4) & 0x1)
#define PS5_5_PIN                  GPIO_PIN_RF4

/*** Macros for PS5_4 pin ***/
#define PS5_4_Set()               (LATFSET = (1<<5))
#define PS5_4_Clear()             (LATFCLR = (1<<5))
#define PS5_4_Toggle()            (LATFINV= (1<<5))
#define PS5_4_OutputEnable()      (TRISFCLR = (1<<5))
#define PS5_4_InputEnable()       (TRISFSET = (1<<5))
#define PS5_4_Get()               ((PORTF >> 5) & 0x1)
#define PS5_4_PIN                  GPIO_PIN_RF5

/*** Macros for PS5_2 pin ***/
#define PS5_2_Set()               (LATFSET = (1<<3))
#define PS5_2_Clear()             (LATFCLR = (1<<3))
#define PS5_2_Toggle()            (LATFINV= (1<<3))
#define PS5_2_OutputEnable()      (TRISFCLR = (1<<3))
#define PS5_2_InputEnable()       (TRISFSET = (1<<3))
#define PS5_2_Get()               ((PORTF >> 3) & 0x1)
#define PS5_2_PIN                  GPIO_PIN_RF3

/*** Macros for PS5_3 pin ***/
#define PS5_3_Set()               (LATASET = (1<<2))
#define PS5_3_Clear()             (LATACLR = (1<<2))
#define PS5_3_Toggle()            (LATAINV= (1<<2))
#define PS5_3_OutputEnable()      (TRISACLR = (1<<2))
#define PS5_3_InputEnable()       (TRISASET = (1<<2))
#define PS5_3_Get()               ((PORTA >> 2) & 0x1)
#define PS5_3_PIN                  GPIO_PIN_RA2

/*** Macros for PS4_6 pin ***/
#define PS4_6_Set()               (LATASET = (1<<3))
#define PS4_6_Clear()             (LATACLR = (1<<3))
#define PS4_6_Toggle()            (LATAINV= (1<<3))
#define PS4_6_OutputEnable()      (TRISACLR = (1<<3))
#define PS4_6_InputEnable()       (TRISASET = (1<<3))
#define PS4_6_Get()               ((PORTA >> 3) & 0x1)
#define PS4_6_PIN                  GPIO_PIN_RA3

/*** Macros for PS4_5 pin ***/
#define PS4_5_Set()               (LATASET = (1<<4))
#define PS4_5_Clear()             (LATACLR = (1<<4))
#define PS4_5_Toggle()            (LATAINV= (1<<4))
#define PS4_5_OutputEnable()      (TRISACLR = (1<<4))
#define PS4_5_InputEnable()       (TRISASET = (1<<4))
#define PS4_5_Get()               ((PORTA >> 4) & 0x1)
#define PS4_5_PIN                  GPIO_PIN_RA4

/*** Macros for PS4_4 pin ***/
#define PS4_4_Set()               (LATASET = (1<<5))
#define PS4_4_Clear()             (LATACLR = (1<<5))
#define PS4_4_Toggle()            (LATAINV= (1<<5))
#define PS4_4_OutputEnable()      (TRISACLR = (1<<5))
#define PS4_4_InputEnable()       (TRISASET = (1<<5))
#define PS4_4_Get()               ((PORTA >> 5) & 0x1)
#define PS4_4_PIN                  GPIO_PIN_RA5

/*** Macros for PS4_3 pin ***/
#define PS4_3_Set()               (LATCSET = (1<<15))
#define PS4_3_Clear()             (LATCCLR = (1<<15))
#define PS4_3_Toggle()            (LATCINV= (1<<15))
#define PS4_3_OutputEnable()      (TRISCCLR = (1<<15))
#define PS4_3_InputEnable()       (TRISCSET = (1<<15))
#define PS4_3_Get()               ((PORTC >> 15) & 0x1)
#define PS4_3_PIN                  GPIO_PIN_RC15

/*** Macros for PS4_2 pin ***/
#define PS4_2_Set()               (LATASET = (1<<14))
#define PS4_2_Clear()             (LATACLR = (1<<14))
#define PS4_2_Toggle()            (LATAINV= (1<<14))
#define PS4_2_OutputEnable()      (TRISACLR = (1<<14))
#define PS4_2_InputEnable()       (TRISASET = (1<<14))
#define PS4_2_Get()               ((PORTA >> 14) & 0x1)
#define PS4_2_PIN                  GPIO_PIN_RA14

/*** Macros for PS4_1 pin ***/
#define PS4_1_Set()               (LATDSET = (1<<9))
#define PS4_1_Clear()             (LATDCLR = (1<<9))
#define PS4_1_Toggle()            (LATDINV= (1<<9))
#define PS4_1_OutputEnable()      (TRISDCLR = (1<<9))
#define PS4_1_InputEnable()       (TRISDSET = (1<<9))
#define PS4_1_Get()               ((PORTD >> 9) & 0x1)
#define PS4_1_PIN                  GPIO_PIN_RD9

/*** Macros for PS3_6 pin ***/
#define PS3_6_Set()               (LATDSET = (1<<10))
#define PS3_6_Clear()             (LATDCLR = (1<<10))
#define PS3_6_Toggle()            (LATDINV= (1<<10))
#define PS3_6_OutputEnable()      (TRISDCLR = (1<<10))
#define PS3_6_InputEnable()       (TRISDSET = (1<<10))
#define PS3_6_Get()               ((PORTD >> 10) & 0x1)
#define PS3_6_PIN                  GPIO_PIN_RD10

/*** Macros for PS3_5 pin ***/
#define PS3_5_Set()               (LATDSET = (1<<0))
#define PS3_5_Clear()             (LATDCLR = (1<<0))
#define PS3_5_Toggle()            (LATDINV= (1<<0))
#define PS3_5_OutputEnable()      (TRISDCLR = (1<<0))
#define PS3_5_InputEnable()       (TRISDSET = (1<<0))
#define PS3_5_Get()               ((PORTD >> 0) & 0x1)
#define PS3_5_PIN                  GPIO_PIN_RD0

/*** Macros for PS3_4 pin ***/
#define PS3_4_Set()               (LATCSET = (1<<13))
#define PS3_4_Clear()             (LATCCLR = (1<<13))
#define PS3_4_Toggle()            (LATCINV= (1<<13))
#define PS3_4_OutputEnable()      (TRISCCLR = (1<<13))
#define PS3_4_InputEnable()       (TRISCSET = (1<<13))
#define PS3_4_Get()               ((PORTC >> 13) & 0x1)
#define PS3_4_PIN                  GPIO_PIN_RC13

/*** Macros for PS3_3 pin ***/
#define PS3_3_Set()               (LATCSET = (1<<14))
#define PS3_3_Clear()             (LATCCLR = (1<<14))
#define PS3_3_Toggle()            (LATCINV= (1<<14))
#define PS3_3_OutputEnable()      (TRISCCLR = (1<<14))
#define PS3_3_InputEnable()       (TRISCSET = (1<<14))
#define PS3_3_Get()               ((PORTC >> 14) & 0x1)
#define PS3_3_PIN                  GPIO_PIN_RC14

/*** Macros for SSR_FAN pin ***/
#define SSR_FAN_Set()               (LATDSET = (1<<1))
#define SSR_FAN_Clear()             (LATDCLR = (1<<1))
#define SSR_FAN_Toggle()            (LATDINV= (1<<1))
#define SSR_FAN_OutputEnable()      (TRISDCLR = (1<<1))
#define SSR_FAN_InputEnable()       (TRISDSET = (1<<1))
#define SSR_FAN_Get()               ((PORTD >> 1) & 0x1)
#define SSR_FAN_PIN                  GPIO_PIN_RD1

/*** Macros for EN_2 pin ***/
#define EN_2_Set()               (LATDSET = (1<<2))
#define EN_2_Clear()             (LATDCLR = (1<<2))
#define EN_2_Toggle()            (LATDINV= (1<<2))
#define EN_2_OutputEnable()      (TRISDCLR = (1<<2))
#define EN_2_InputEnable()       (TRISDSET = (1<<2))
#define EN_2_Get()               ((PORTD >> 2) & 0x1)
#define EN_2_PIN                  GPIO_PIN_RD2

/*** Macros for SW_3 pin ***/
#define SW_3_Set()               (LATDSET = (1<<3))
#define SW_3_Clear()             (LATDCLR = (1<<3))
#define SW_3_Toggle()            (LATDINV= (1<<3))
#define SW_3_OutputEnable()      (TRISDCLR = (1<<3))
#define SW_3_InputEnable()       (TRISDSET = (1<<3))
#define SW_3_Get()               ((PORTD >> 3) & 0x1)
#define SW_3_PIN                  GPIO_PIN_RD3

/*** Macros for SW_2 pin ***/
#define SW_2_Set()               (LATDSET = (1<<12))
#define SW_2_Clear()             (LATDCLR = (1<<12))
#define SW_2_Toggle()            (LATDINV= (1<<12))
#define SW_2_OutputEnable()      (TRISDCLR = (1<<12))
#define SW_2_InputEnable()       (TRISDSET = (1<<12))
#define SW_2_Get()               ((PORTD >> 12) & 0x1)
#define SW_2_PIN                  GPIO_PIN_RD12

/*** Macros for SW_4 pin ***/
#define SW_4_Set()               (LATDSET = (1<<13))
#define SW_4_Clear()             (LATDCLR = (1<<13))
#define SW_4_Toggle()            (LATDINV= (1<<13))
#define SW_4_OutputEnable()      (TRISDCLR = (1<<13))
#define SW_4_InputEnable()       (TRISDSET = (1<<13))
#define SW_4_Get()               ((PORTD >> 13) & 0x1)
#define SW_4_PIN                  GPIO_PIN_RD13

/*** Macros for SW_1 pin ***/
#define SW_1_Set()               (LATDSET = (1<<4))
#define SW_1_Clear()             (LATDCLR = (1<<4))
#define SW_1_Toggle()            (LATDINV= (1<<4))
#define SW_1_OutputEnable()      (TRISDCLR = (1<<4))
#define SW_1_InputEnable()       (TRISDSET = (1<<4))
#define SW_1_Get()               ((PORTD >> 4) & 0x1)
#define SW_1_PIN                  GPIO_PIN_RD4

/*** Macros for EN_3 pin ***/
#define EN_3_Set()               (LATDSET = (1<<5))
#define EN_3_Clear()             (LATDCLR = (1<<5))
#define EN_3_Toggle()            (LATDINV= (1<<5))
#define EN_3_OutputEnable()      (TRISDCLR = (1<<5))
#define EN_3_InputEnable()       (TRISDSET = (1<<5))
#define EN_3_Get()               ((PORTD >> 5) & 0x1)
#define EN_3_PIN                  GPIO_PIN_RD5

/*** Macros for EN_1 pin ***/
#define EN_1_Set()               (LATDSET = (1<<6))
#define EN_1_Clear()             (LATDCLR = (1<<6))
#define EN_1_Toggle()            (LATDINV= (1<<6))
#define EN_1_OutputEnable()      (TRISDCLR = (1<<6))
#define EN_1_InputEnable()       (TRISDSET = (1<<6))
#define EN_1_Get()               ((PORTD >> 6) & 0x1)
#define EN_1_PIN                  GPIO_PIN_RD6

/*** Macros for EN_4 pin ***/
#define EN_4_Set()               (LATDSET = (1<<7))
#define EN_4_Clear()             (LATDCLR = (1<<7))
#define EN_4_Toggle()            (LATDINV= (1<<7))
#define EN_4_OutputEnable()      (TRISDCLR = (1<<7))
#define EN_4_InputEnable()       (TRISDSET = (1<<7))
#define EN_4_Get()               ((PORTD >> 7) & 0x1)
#define EN_4_PIN                  GPIO_PIN_RD7

/*** Macros for PS3_2 pin ***/
#define PS3_2_Set()               (LATFSET = (1<<0))
#define PS3_2_Clear()             (LATFCLR = (1<<0))
#define PS3_2_Toggle()            (LATFINV= (1<<0))
#define PS3_2_OutputEnable()      (TRISFCLR = (1<<0))
#define PS3_2_InputEnable()       (TRISFSET = (1<<0))
#define PS3_2_Get()               ((PORTF >> 0) & 0x1)
#define PS3_2_PIN                  GPIO_PIN_RF0

/*** Macros for PS3_1 pin ***/
#define PS3_1_Set()               (LATFSET = (1<<1))
#define PS3_1_Clear()             (LATFCLR = (1<<1))
#define PS3_1_Toggle()            (LATFINV= (1<<1))
#define PS3_1_OutputEnable()      (TRISFCLR = (1<<1))
#define PS3_1_InputEnable()       (TRISFSET = (1<<1))
#define PS3_1_Get()               ((PORTF >> 1) & 0x1)
#define PS3_1_PIN                  GPIO_PIN_RF1

/*** Macros for PS2_6 pin ***/
#define PS2_6_Set()               (LATGSET = (1<<1))
#define PS2_6_Clear()             (LATGCLR = (1<<1))
#define PS2_6_Toggle()            (LATGINV= (1<<1))
#define PS2_6_OutputEnable()      (TRISGCLR = (1<<1))
#define PS2_6_InputEnable()       (TRISGSET = (1<<1))
#define PS2_6_Get()               ((PORTG >> 1) & 0x1)
#define PS2_6_PIN                  GPIO_PIN_RG1

/*** Macros for PS2_5 pin ***/
#define PS2_5_Set()               (LATGSET = (1<<0))
#define PS2_5_Clear()             (LATGCLR = (1<<0))
#define PS2_5_Toggle()            (LATGINV= (1<<0))
#define PS2_5_OutputEnable()      (TRISGCLR = (1<<0))
#define PS2_5_InputEnable()       (TRISGSET = (1<<0))
#define PS2_5_Get()               ((PORTG >> 0) & 0x1)
#define PS2_5_PIN                  GPIO_PIN_RG0

/*** Macros for PS2_4 pin ***/
#define PS2_4_Set()               (LATASET = (1<<6))
#define PS2_4_Clear()             (LATACLR = (1<<6))
#define PS2_4_Toggle()            (LATAINV= (1<<6))
#define PS2_4_OutputEnable()      (TRISACLR = (1<<6))
#define PS2_4_InputEnable()       (TRISASET = (1<<6))
#define PS2_4_Get()               ((PORTA >> 6) & 0x1)
#define PS2_4_PIN                  GPIO_PIN_RA6

/*** Macros for PS2_3 pin ***/
#define PS2_3_Set()               (LATASET = (1<<7))
#define PS2_3_Clear()             (LATACLR = (1<<7))
#define PS2_3_Toggle()            (LATAINV= (1<<7))
#define PS2_3_OutputEnable()      (TRISACLR = (1<<7))
#define PS2_3_InputEnable()       (TRISASET = (1<<7))
#define PS2_3_Get()               ((PORTA >> 7) & 0x1)
#define PS2_3_PIN                  GPIO_PIN_RA7

/*** Macros for PS2_2 pin ***/
#define PS2_2_Set()               (LATESET = (1<<0))
#define PS2_2_Clear()             (LATECLR = (1<<0))
#define PS2_2_Toggle()            (LATEINV= (1<<0))
#define PS2_2_OutputEnable()      (TRISECLR = (1<<0))
#define PS2_2_InputEnable()       (TRISESET = (1<<0))
#define PS2_2_Get()               ((PORTE >> 0) & 0x1)
#define PS2_2_PIN                  GPIO_PIN_RE0

/*** Macros for PS2_1 pin ***/
#define PS2_1_Set()               (LATESET = (1<<1))
#define PS2_1_Clear()             (LATECLR = (1<<1))
#define PS2_1_Toggle()            (LATEINV= (1<<1))
#define PS2_1_OutputEnable()      (TRISECLR = (1<<1))
#define PS2_1_InputEnable()       (TRISESET = (1<<1))
#define PS2_1_Get()               ((PORTE >> 1) & 0x1)
#define PS2_1_PIN                  GPIO_PIN_RE1

/*** Macros for PS1_6 pin ***/
#define PS1_6_Set()               (LATGSET = (1<<14))
#define PS1_6_Clear()             (LATGCLR = (1<<14))
#define PS1_6_Toggle()            (LATGINV= (1<<14))
#define PS1_6_OutputEnable()      (TRISGCLR = (1<<14))
#define PS1_6_InputEnable()       (TRISGSET = (1<<14))
#define PS1_6_Get()               ((PORTG >> 14) & 0x1)
#define PS1_6_PIN                  GPIO_PIN_RG14

/*** Macros for PS1_5 pin ***/
#define PS1_5_Set()               (LATGSET = (1<<12))
#define PS1_5_Clear()             (LATGCLR = (1<<12))
#define PS1_5_Toggle()            (LATGINV= (1<<12))
#define PS1_5_OutputEnable()      (TRISGCLR = (1<<12))
#define PS1_5_InputEnable()       (TRISGSET = (1<<12))
#define PS1_5_Get()               ((PORTG >> 12) & 0x1)
#define PS1_5_PIN                  GPIO_PIN_RG12

/*** Macros for PS1_4 pin ***/
#define PS1_4_Set()               (LATGSET = (1<<13))
#define PS1_4_Clear()             (LATGCLR = (1<<13))
#define PS1_4_Toggle()            (LATGINV= (1<<13))
#define PS1_4_OutputEnable()      (TRISGCLR = (1<<13))
#define PS1_4_InputEnable()       (TRISGSET = (1<<13))
#define PS1_4_Get()               ((PORTG >> 13) & 0x1)
#define PS1_4_PIN                  GPIO_PIN_RG13

/*** Macros for PS1_3 pin ***/
#define PS1_3_Set()               (LATESET = (1<<2))
#define PS1_3_Clear()             (LATECLR = (1<<2))
#define PS1_3_Toggle()            (LATEINV= (1<<2))
#define PS1_3_OutputEnable()      (TRISECLR = (1<<2))
#define PS1_3_InputEnable()       (TRISESET = (1<<2))
#define PS1_3_Get()               ((PORTE >> 2) & 0x1)
#define PS1_3_PIN                  GPIO_PIN_RE2

/*** Macros for PS1_2 pin ***/
#define PS1_2_Set()               (LATESET = (1<<3))
#define PS1_2_Clear()             (LATECLR = (1<<3))
#define PS1_2_Toggle()            (LATEINV= (1<<3))
#define PS1_2_OutputEnable()      (TRISECLR = (1<<3))
#define PS1_2_InputEnable()       (TRISESET = (1<<3))
#define PS1_2_Get()               ((PORTE >> 3) & 0x1)
#define PS1_2_PIN                  GPIO_PIN_RE3

/*** Macros for PS1_1 pin ***/
#define PS1_1_Set()               (LATESET = (1<<4))
#define PS1_1_Clear()             (LATECLR = (1<<4))
#define PS1_1_Toggle()            (LATEINV= (1<<4))
#define PS1_1_OutputEnable()      (TRISECLR = (1<<4))
#define PS1_1_InputEnable()       (TRISESET = (1<<4))
#define PS1_1_Get()               ((PORTE >> 4) & 0x1)
#define PS1_1_PIN                  GPIO_PIN_RE4


// *****************************************************************************
/* GPIO Port

  Summary:
    Identifies the available GPIO Ports.

  Description:
    This enumeration identifies the available GPIO Ports.

  Remarks:
    The caller should not rely on the specific numbers assigned to any of
    these values as they may change from one processor to the next.

    Not all ports are available on all devices.  Refer to the specific
    device data sheet to determine which ports are supported.
*/

typedef enum
{
    GPIO_PORT_A = 0,
    GPIO_PORT_B = 1,
    GPIO_PORT_C = 2,
    GPIO_PORT_D = 3,
    GPIO_PORT_E = 4,
    GPIO_PORT_F = 5,
    GPIO_PORT_G = 6,
} GPIO_PORT;

// *****************************************************************************
/* GPIO Port Pins

  Summary:
    Identifies the available GPIO port pins.

  Description:
    This enumeration identifies the available GPIO port pins.

  Remarks:
    The caller should not rely on the specific numbers assigned to any of
    these values as they may change from one processor to the next.

    Not all pins are available on all devices.  Refer to the specific
    device data sheet to determine which pins are supported.
*/

typedef enum
{
    GPIO_PIN_RA0 = 0,
    GPIO_PIN_RA1 = 1,
    GPIO_PIN_RA2 = 2,
    GPIO_PIN_RA3 = 3,
    GPIO_PIN_RA4 = 4,
    GPIO_PIN_RA5 = 5,
    GPIO_PIN_RA6 = 6,
    GPIO_PIN_RA7 = 7,
    GPIO_PIN_RA9 = 9,
    GPIO_PIN_RA10 = 10,
    GPIO_PIN_RA14 = 14,
    GPIO_PIN_RA15 = 15,
    GPIO_PIN_RB0 = 16,
    GPIO_PIN_RB1 = 17,
    GPIO_PIN_RB2 = 18,
    GPIO_PIN_RB3 = 19,
    GPIO_PIN_RB4 = 20,
    GPIO_PIN_RB5 = 21,
    GPIO_PIN_RB6 = 22,
    GPIO_PIN_RB7 = 23,
    GPIO_PIN_RB8 = 24,
    GPIO_PIN_RB9 = 25,
    GPIO_PIN_RB10 = 26,
    GPIO_PIN_RB11 = 27,
    GPIO_PIN_RB12 = 28,
    GPIO_PIN_RB13 = 29,
    GPIO_PIN_RB14 = 30,
    GPIO_PIN_RB15 = 31,
    GPIO_PIN_RC1 = 33,
    GPIO_PIN_RC2 = 34,
    GPIO_PIN_RC3 = 35,
    GPIO_PIN_RC4 = 36,
    GPIO_PIN_RC12 = 44,
    GPIO_PIN_RC13 = 45,
    GPIO_PIN_RC14 = 46,
    GPIO_PIN_RC15 = 47,
    GPIO_PIN_RD0 = 48,
    GPIO_PIN_RD1 = 49,
    GPIO_PIN_RD2 = 50,
    GPIO_PIN_RD3 = 51,
    GPIO_PIN_RD4 = 52,
    GPIO_PIN_RD5 = 53,
    GPIO_PIN_RD6 = 54,
    GPIO_PIN_RD7 = 55,
    GPIO_PIN_RD8 = 56,
    GPIO_PIN_RD9 = 57,
    GPIO_PIN_RD10 = 58,
    GPIO_PIN_RD11 = 59,
    GPIO_PIN_RD12 = 60,
    GPIO_PIN_RD13 = 61,
    GPIO_PIN_RD14 = 62,
    GPIO_PIN_RD15 = 63,
    GPIO_PIN_RE0 = 64,
    GPIO_PIN_RE1 = 65,
    GPIO_PIN_RE2 = 66,
    GPIO_PIN_RE3 = 67,
    GPIO_PIN_RE4 = 68,
    GPIO_PIN_RE5 = 69,
    GPIO_PIN_RE6 = 70,
    GPIO_PIN_RE7 = 71,
    GPIO_PIN_RE8 = 72,
    GPIO_PIN_RE9 = 73,
    GPIO_PIN_RF0 = 80,
    GPIO_PIN_RF1 = 81,
    GPIO_PIN_RF2 = 82,
    GPIO_PIN_RF3 = 83,
    GPIO_PIN_RF4 = 84,
    GPIO_PIN_RF5 = 85,
    GPIO_PIN_RF8 = 88,
    GPIO_PIN_RF12 = 92,
    GPIO_PIN_RF13 = 93,
    GPIO_PIN_RG0 = 96,
    GPIO_PIN_RG1 = 97,
    GPIO_PIN_RG2 = 98,
    GPIO_PIN_RG3 = 99,
    GPIO_PIN_RG6 = 102,
    GPIO_PIN_RG7 = 103,
    GPIO_PIN_RG8 = 104,
    GPIO_PIN_RG9 = 105,
    GPIO_PIN_RG12 = 108,
    GPIO_PIN_RG13 = 109,
    GPIO_PIN_RG14 = 110,
    GPIO_PIN_RG15 = 111,

    /* This element should not be used in any of the GPIO APIs.
       It will be used by other modules or application to denote that none of the GPIO Pin is used */
    GPIO_PIN_NONE = -1

} GPIO_PIN;

typedef enum
{
  CN0_PIN = 1 << 0,
  CN1_PIN = 1 << 1,
  CN2_PIN = 1 << 2,
  CN3_PIN = 1 << 3,
  CN4_PIN = 1 << 4,
  CN5_PIN = 1 << 5,
  CN6_PIN = 1 << 6,
  CN7_PIN = 1 << 7,
  CN8_PIN = 1 << 8,
  CN9_PIN = 1 << 9,
  CN10_PIN = 1 << 10,
  CN11_PIN = 1 << 11,
  CN12_PIN = 1 << 12,
  CN13_PIN = 1 << 13,
  CN14_PIN = 1 << 14,
  CN15_PIN = 1 << 15,
  CN16_PIN = 1 << 16,
  CN17_PIN = 1 << 17,
  CN18_PIN = 1 << 18,
  CN19_PIN = 1 << 19,
  CN20_PIN = 1 << 20,
  CN21_PIN = 1 << 21,
}CN_PIN;


void GPIO_Initialize(void);

// *****************************************************************************
// *****************************************************************************
// Section: GPIO Functions which operates on multiple pins of a port
// *****************************************************************************
// *****************************************************************************

uint32_t GPIO_PortRead(GPIO_PORT port);

void GPIO_PortWrite(GPIO_PORT port, uint32_t mask, uint32_t value);

uint32_t GPIO_PortLatchRead ( GPIO_PORT port );

void GPIO_PortSet(GPIO_PORT port, uint32_t mask);

void GPIO_PortClear(GPIO_PORT port, uint32_t mask);

void GPIO_PortToggle(GPIO_PORT port, uint32_t mask);

void GPIO_PortInputEnable(GPIO_PORT port, uint32_t mask);

void GPIO_PortOutputEnable(GPIO_PORT port, uint32_t mask);

// *****************************************************************************
// *****************************************************************************
// Section: GPIO Functions which operates on one pin at a time
// *****************************************************************************
// *****************************************************************************

static inline void GPIO_PinWrite(GPIO_PIN pin, bool value)
{
    GPIO_PortWrite((GPIO_PORT)(pin>>4), (uint32_t)(0x1) << (pin & 0xF), (uint32_t)(value) << (pin & 0xF));
}

static inline bool GPIO_PinRead(GPIO_PIN pin)
{
    return (bool)(((GPIO_PortRead((GPIO_PORT)(pin>>4))) >> (pin & 0xF)) & 0x1);
}

static inline bool GPIO_PinLatchRead(GPIO_PIN pin)
{
    return (bool)((GPIO_PortLatchRead((GPIO_PORT)(pin>>4)) >> (pin & 0xF)) & 0x1);
}

static inline void GPIO_PinToggle(GPIO_PIN pin)
{
    GPIO_PortToggle((GPIO_PORT)(pin>>4), 0x1 << (pin & 0xF));
}

static inline void GPIO_PinSet(GPIO_PIN pin)
{
    GPIO_PortSet((GPIO_PORT)(pin>>4), 0x1 << (pin & 0xF));
}

static inline void GPIO_PinClear(GPIO_PIN pin)
{
    GPIO_PortClear((GPIO_PORT)(pin>>4), 0x1 << (pin & 0xF));
}

static inline void GPIO_PinInputEnable(GPIO_PIN pin)
{
    GPIO_PortInputEnable((GPIO_PORT)(pin>>4), 0x1 << (pin & 0xF));
}

static inline void GPIO_PinOutputEnable(GPIO_PIN pin)
{
    GPIO_PortOutputEnable((GPIO_PORT)(pin>>4), 0x1 << (pin & 0xF));
}


// DOM-IGNORE-BEGIN
#ifdef __cplusplus  // Provide C++ Compatibility

    }

#endif
// DOM-IGNORE-END
#endif // PLIB_GPIO_H
