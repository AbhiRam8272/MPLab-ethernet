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

/*******************************************************************************
  MPLAB Harmony Application Header File

  Company:
    Microchip Technology Inc.

  File Name:
    app.h

  Summary:
    This header file provides prototypes and definitions for the application.

  Description:
    This header file provides function prototypes and data type definitions for
    the application.  Some of these are required by the system (such as the
    "APP_Initialize" and "APP_Tasks" prototypes) and some of them are only used
    internally by the application (such as the "APP_STATES" definition).  Both
    are defined here for convenience.
*******************************************************************************/

#ifndef _APP_H
#define _APP_H

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include "configuration.h"
#include "definitions.h"

#define MAX_CLIENT (3)
#define bits_to_transfer    24                                          // For LMX device, the register is packed with parameter's address and value in it. So <23:16> for address and <15:0> for data.
#define bits_to_transfer_for_read        8                          // while reading address value should be sent. this #define will be helpful there
#define bits_to_read        16                                          // during the read operation, we need to monitor SDI pin status for 16 cycles
#define MSB_MASK_24bit      0x800000                        // to mask the 23rd bit position of any value
#define testbit(var,bit)    (var &= (1UL << bit))
#define getbit(var, bit)    (var = testbit(var,bit) >> bit)
#define setbit(var,bit)     (var |= (1UL << bit))                       // To set a bit position
#define clearbit(var,bit)   (var &= (~(1UL << bit)))                   // To clear a bit position
#define togglebit(var, bit) (var ^= (1UL << bit))                       // To toggle a bit position
#define PLL_regs_arr_size           113UL                       // Number of PLL registers in LMX2594
#define PLL1_ID                         0                         // ID of LMX 1
#define PLL2_ID                         1UL                         // ID of LMX 2
#define SCK_HIGH            (setbit(LATD, 0))                   // setting serial clock functionality pin of MCU as high
#define SCK_LOW             (clearbit(LATD, 0))                 // setting serial clock functionality pin of MCU as low
#define SDO_HIGH            (setbit(LATD, 10)) 			// setting serial data out functionality pin of MCU as high	
#define SDO_LOW             (clearbit(LATD, 10))			// setting serial data out functionality pin of MCU as low
#define LE_1_HIGH             (setbit(LATD, 9)) 		// setting chip select pin(GPIO) of MCU as high	
#define LE_1_LOW              (clearbit(LATD, 9))		// setting chip select pin(GPIO) of MCU as low		
//#define LE_2_HIGH             (setbit(LATG, 14)) 		// setting chip select pin(GPIO) of MCU as high	
//#define LE_2_LOW              (clearbit(LATG, 14))		// setting chip select pin(GPIO) of MCU as low	
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
#define MIN_N                   36UL                   // Minimum interger value 
#define REF_IN                  100e3                   // 100MHz Reference input clock
#define PFD                         100e3                   // Phase detector frequency is set as 100MHz
#define F_VCO_MIN           7.5e6                   // LMX2595 device's min VCO frequency
#define F_VCO_MAX           15e6                    // LMX2595 device's max VCO frequency
#define fractional_denom        4294967296UL    // Considered this Fractional denominator value for calculations
#define CHANNELA                0                       // To get output from channel A of LMX2594. 
#define CHANNELB                1UL                    // To get output from channel B of LMX2594. 
#define CHANNEL_BOTH        2UL                 // To get output from both channels A & B of LMX2594
#define CHANNEL_NONE        3UL                 // To get no output from both channels A & B of LMX2594
#define MIN_FREQ                10.0e3                   // Minimum frequency
#define MAX_FREQ                15.0e6                    // Maximum frequency 
#define N_DIV_RATIOS  18UL
// *****************************************************************************
// *****************************************************************************
// Section: Type Definitions
// *****************************************************************************
// *****************************************************************************

// *****************************************************************************
/* Application States

  Summary:
    Application states enumeration

  Description:
    This enumeration defines the valid application states.  These states
    determine the behavior of the application at various times.
*/

typedef enum
{
    APP_TCPIP_WAIT_INIT,
	/* Application's state machine's initial state. */
    APP_TCPIP_WAIT_FOR_IP,
    APP_BSD_INIT,
    APP_BSD_CREATE_SOCKET,
    APP_BSD_BIND,
    APP_BSD_LISTEN,
    APP_BSD_OPERATION,
    APP_TCPIP_ERROR,

} APP_STATES;


// *****************************************************************************
/* Application Data

  Summary:
    Holds application data

  Description:
    This structure holds the application's data.

  Remarks:
    Application strings and buffers are be defined outside this structure.
 */

typedef struct
{
    /* The application's current state */
    APP_STATES state;

    /* TODO: Define any additional data used by the application. */
    SOCKET bsdServerSocket;
    SOCKET ClientSock[MAX_CLIENT];


} APP_DATA;


// *****************************************************************************
// *****************************************************************************
// Section: Application Callback Routines
// *****************************************************************************
// *****************************************************************************
/* These routines are called by drivers when certain events occur.
*/

	
// *****************************************************************************
// *****************************************************************************
// Section: Application Initialization and State Machine Functions
// *****************************************************************************
// *****************************************************************************

/*******************************************************************************
  Function:
    void APP_Initialize ( void )

  Summary:
     MPLAB Harmony application initialization routine.

  Description:
    This function initializes the Harmony application.  It places the 
    application in its initial state and prepares it to run so that its 
    APP_Tasks function can be called.

  Precondition:
    All other system initialization routines should be called before calling
    this routine (in "SYS_Initialize").

  Parameters:
    None.

  Returns:
    None.

  Example:
    <code>
    APP_Initialize();
    </code>

  Remarks:
    This routine must be called from the SYS_Initialize function.
*/

void APP_Initialize ( void );


/*******************************************************************************
  Function:
    void APP_Tasks ( void )

  Summary:
    MPLAB Harmony Demo application tasks function

  Description:
    This routine is the Harmony Demo application's tasks function.  It
    defines the application's state machine and core logic.

  Precondition:
    The system and application initialization ("SYS_Initialize") should be
    called before calling this.

  Parameters:
    None.

  Returns:
    None.

  Example:
    <code>
    APP_Tasks();
    </code>

  Remarks:
    This routine must be called from SYS_Tasks() routine.
 */

void APP_Tasks ( void );

void LMX_WRITE(unsigned int SPI_line, unsigned long array_element);
unsigned long LMX_READ(unsigned int SPI_line, unsigned char read_address);
void Load_registers(unsigned int SPI_line, unsigned long array[2][PLL_regs_arr_size]);
void LMX_PLL_regs_write(unsigned char PLL_ID);
unsigned long calc_n(double f, double n_step,unsigned long div);
void LMX_set_chan_power(unsigned int PLL_ID, unsigned char channel, unsigned char power);
void LMX_set_freq(unsigned char PLL_ID, unsigned long f);

#endif /* _APP_H */
/*******************************************************************************
 End of File
 */