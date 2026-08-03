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
  MPLAB Harmony Application Source File

  Company:
    Microchip Technology Inc.

  File Name:
    app.c

  Summary:
    This file contains the source code for the MPLAB Harmony application.

  Description:
    This file contains the source code for the MPLAB Harmony application.  It
    implements the logic of the application's state machine and it may call
    API routines of other MPLAB Harmony modules in the system, such as drivers,
    system services, and middleware.  However, it does not call any of the
    system interfaces (such as the "Initialize" and "Tasks" functions) of any of
    the modules in the system or make any assumptions about when those functions
    are called.  That is the responsibility of the configuration-specific system
    files.
 *******************************************************************************/

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************

#include "app.h"
#include "peripheral/gpio/plib_gpio.h"
#include "tcpip/tcpip.h"

#include <errno.h>

#define SERVER_PORT 5001


// *****************************************************************************
// *****************************************************************************
// Section: Global Data Definitions
// *****************************************************************************
// *****************************************************************************
char version_str[] = "@@V1.0%%";               // string that need to be replied to client when version command frame is received
char io_pack_ack_str[] = "OK";               // string that need to be replied to client when version command frame is received
// *****************************************************************************
/* Application Data

  Summary:
    Holds application data

  Description:
    This structure holds the application's data.

  Remarks:
    This structure should be initialized by the APP_Initialize function.
    
    Application strings and buffers are be defined outside this structure.
 */

APP_DATA appData;


// *****************************************************************************
// *****************************************************************************
// Section: Application Callback Functions
// *****************************************************************************
// *****************************************************************************

/* TODO:  Add any necessary callback functions.
 */


// *****************************************************************************
// *****************************************************************************
// Section: Application Local Functions
// *****************************************************************************
// *****************************************************************************


// *****************************************************************************
// *****************************************************************************
// Section: Application Initialization and State Machine Functions
// *****************************************************************************
// *****************************************************************************

/*******************************************************************************
  Function:
    void APP_Initialize ( void )

  Remarks:
    See prototype in app.h.
 */

void APP_Initialize(void) {
    /* Place the App state machine in its initial state. */
    appData.state = APP_TCPIP_WAIT_INIT;

    /* TODO: Initialize your application's state machine and other
     * parameters.
     */
}

/******************************************************************************
  Function:
    void APP_Tasks ( void )

  Remarks:
    See prototype in app.h.
 */

void APP_Tasks(void) {
    SYS_STATUS tcpipStat;
    const char *netName, *netBiosName;
    int i, nNets;
    TCPIP_NET_HANDLE netH;


    switch (appData.state) {
        case APP_TCPIP_WAIT_INIT:
            tcpipStat = TCPIP_STACK_Status(sysObj.tcpip);
            if (tcpipStat < 0) { // some error occurred
                SYS_CONSOLE_MESSAGE(" APP: TCP/IP stack initialization failed!\r\n");
                appData.state = APP_TCPIP_ERROR;
            } else if (tcpipStat == SYS_STATUS_READY) {
                // now that the stack is ready we can check the 
                // available interfaces
                nNets = TCPIP_STACK_NumberOfNetworksGet();

                for (i = 0; i < nNets; i++) {

                    netH = TCPIP_STACK_IndexToNet(i);
                    netName = TCPIP_STACK_NetNameGet(netH);
                    netBiosName = TCPIP_STACK_NetBIOSName(netH);

#if defined(TCPIP_STACK_USE_NBNS)
                    SYS_CONSOLE_PRINT("    Interface %s on host %s - NBNS enabled\r\n", netName, netBiosName);
#else
                    SYS_CONSOLE_PRINT("    Interface %s on host %s - NBNS disabled\r\n", netName, netBiosName);
#endif  // defined(TCPIP_STACK_USE_NBNS)
                    (void)netName;          // avoid compiler warning 
                    (void)netBiosName;      // if SYS_CONSOLE_PRINT is null macro

                }

                appData.state = APP_TCPIP_WAIT_FOR_IP;
            }

            break;

        case APP_TCPIP_WAIT_FOR_IP:
            nNets = TCPIP_STACK_NumberOfNetworksGet();
            for (i = 0; i < nNets; i++) {
                netH = TCPIP_STACK_IndexToNet(i);
                if (!TCPIP_STACK_NetIsReady(netH)) {
                    return; // interface not ready yet!
                }
                IPV4_ADDR           ipAddr;
                ipAddr.Val = TCPIP_STACK_NetAddress(netH);
                SYS_CONSOLE_MESSAGE(TCPIP_STACK_NetNameGet(netH));
                SYS_CONSOLE_MESSAGE(" IP Address: ");
                SYS_CONSOLE_PRINT("%d.%d.%d.%d \r\n", ipAddr.v[0], ipAddr.v[1], ipAddr.v[2], ipAddr.v[3]);
            }
            // all interfaces ready. Could start transactions!!!
            appData.state = APP_BSD_INIT;
            //... etc.
            break;
        case APP_BSD_INIT:
        {
            // Initialize all client socket handles so that we don't process
            // them in the BSD_OPERATION state
            for (i = 0; i < MAX_CLIENT; i++)
                appData.ClientSock[i] = INVALID_SOCKET;

            appData.state = APP_BSD_CREATE_SOCKET;

        }
            break;

        case APP_BSD_CREATE_SOCKET:
        {
            // Create a socket for this server to listen and accept connections on
            SOCKET tcpSkt = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (tcpSkt == INVALID_SOCKET)
                return;
            appData.bsdServerSocket = (SOCKET) tcpSkt;

            appData.state = APP_BSD_BIND;
        }
            break;

        case APP_BSD_BIND:
        {
            // Bind socket to a local port
            struct sockaddr_in addr;
            int addrlen = sizeof (struct sockaddr_in);
            addr.sin_port = SERVER_PORT;
            addr.sin_addr.S_un.S_addr = IP_ADDR_ANY;
            if (bind(appData.bsdServerSocket, (struct sockaddr*) &addr, addrlen) == SOCKET_ERROR)
                return;

            appData.state = APP_BSD_LISTEN;
            // No break needed
        }
            break;

        case APP_BSD_LISTEN:
        {
            if (listen(appData.bsdServerSocket, MAX_CLIENT) == 0) {
                appData.state = APP_BSD_OPERATION;
                SYS_CONSOLE_PRINT("Waiting for Client Connection on port: %d\r\n", SERVER_PORT);
            }
        }
            break;
        case APP_BSD_OPERATION:
        {
            int length;
            struct sockaddr_in addRemote;
            int addrlen = sizeof (struct sockaddr_in);
            char bfr[30];

            for (i = 0; i < MAX_CLIENT; i++) 
            {                
                // Accept any pending connection requests, assuming we have a place to store the socket descriptor
                if (appData.ClientSock[i] == INVALID_SOCKET)
                    appData.ClientSock[i] = accept(appData.bsdServerSocket, (struct sockaddr*) &addRemote, &addrlen);

                // If this socket is not connected then no need to process anything
                if (appData.ClientSock[i] == INVALID_SOCKET)
                    continue;

                // For all connected sockets, receive and send back the data
                length = recv(appData.ClientSock[i], bfr, sizeof (bfr), 0);

                if (length > 0) 
                {
                    SYS_CONSOLE_PRINT("%d\r\n", appData.ClientSock[i]);
                    bfr[length] = '\0';
                    if(bfr[length-4] == 'V' && bfr[length-3] == 'E' && bfr[length-2] == 'R' && bfr[length-1] == '?')                                // Version packet frame
                    {
//                        ver_check_flag = 1;                                                                                                                                 // set this flag to give acknowledgment to client
                        memset(bfr, '\0', 30);                                                                                                                    // free up the buffer
                        send(appData.ClientSock[i], version_str, strlen(version_str), 0);
                        length = 0;
                    }      
                    else if(bfr[length-15] == 'I' && bfr[length-14] == 'O' && bfr[length-2] == '*' && bfr[length-1] == '*')       
                    {
                        if((bfr[length-13] & 0x01) >> 0)    PS1_1_Set();
                        else                                PS1_1_Clear();
                        if((bfr[length-13] & 0x02) >> 1)    PS1_2_Set();
                        else                                PS1_2_Clear();
                        if((bfr[length-13] & 0x04) >> 2)    PS1_3_Set();
                        else                                PS1_3_Clear();
                        if((bfr[length-13] & 0x08) >> 3)    PS1_4_Set();
                        else                                PS1_4_Clear();
                        if((bfr[length-13] & 0x10) >> 4)    PS1_5_Set();
                        else                                PS1_5_Clear();
                        if((bfr[length-13] & 0x20) >> 5)    PS1_6_Set();
                        else                                PS1_6_Clear();
                        if((bfr[length-12] & 0x01) >> 0)    PS2_1_Set();
                        else                                PS2_1_Clear();
                        if((bfr[length-12] & 0x02) >> 1)    PS2_2_Set();
                        else                                PS2_2_Clear();
                        if((bfr[length-12] & 0x04) >> 2)    PS2_3_Set();
                        else                                PS2_3_Clear();
                        if((bfr[length-12] & 0x08) >> 3)    PS2_4_Set();
                        else                                PS2_4_Clear();
                        if((bfr[length-12] & 0x10) >> 4)    PS2_5_Set();
                        else                                PS2_5_Clear();
                        if((bfr[length-12] & 0x20) >> 5)    PS2_6_Set();
                        else                                PS2_6_Clear();
                        if((bfr[length-11] & 0x01) >> 0)    PS3_1_Set();
                        else                                PS3_1_Clear();
                        if((bfr[length-11] & 0x02) >> 1)    PS3_2_Set();
                        else                                PS3_2_Clear();
                        if((bfr[length-11] & 0x04) >> 2)    PS3_3_Set();
                        else                                PS3_3_Clear();
                        if((bfr[length-11] & 0x08) >> 3)    PS3_4_Set();
                        else                                PS3_4_Clear();
                        if((bfr[length-11] & 0x10) >> 4)    PS3_5_Set();
                        else                                PS3_5_Clear();
                        if((bfr[length-11] & 0x20) >> 5)    PS3_6_Set();
                        else                                PS3_6_Clear();
                        if((bfr[length-10] & 0x01) >> 0)    PS4_1_Set();
                        else                                PS4_1_Clear();
                        if((bfr[length-10] & 0x02) >> 1)    PS4_2_Set();
                        else                                PS4_2_Clear();
                        if((bfr[length-10] & 0x04) >> 2)    PS4_3_Set();
                        else                                PS4_3_Clear();
                        if((bfr[length-10] & 0x08) >> 3)    PS4_4_Set();
                        else                                PS4_4_Clear();
                        if((bfr[length-10] & 0x10) >> 4)    PS4_5_Set();
                        else                                PS4_5_Clear();
                        if((bfr[length-10] & 0x20) >> 5)    PS4_6_Set();
                        else                                PS4_6_Clear();
                        if((bfr[length-9] & 0x01) >> 0)    PS5_1_Set();
                        else                               PS5_1_Clear();
                        if((bfr[length-9] & 0x02) >> 1)    PS5_2_Set();
                        else                               PS5_2_Clear();
                        if((bfr[length-9] & 0x04) >> 2)    PS5_3_Set();
                        else                               PS5_3_Clear();
                        if((bfr[length-9] & 0x08) >> 3)    PS5_4_Set();
                        else                               PS5_4_Clear();
                        if((bfr[length-9] & 0x10) >> 4)    PS5_5_Set();
                        else                               PS5_5_Clear();
                        if((bfr[length-9] & 0x20) >> 5)    PS5_6_Set();
                        else                               PS5_6_Clear();
                        if((bfr[length-8] & 0x01) >> 0)    PS6_1_Set();
                        else                               PS6_1_Clear();
                        if((bfr[length-8] & 0x02) >> 1)    PS6_2_Set();
                        else                               PS6_2_Clear();
                        if((bfr[length-8] & 0x04) >> 2)    PS6_3_Set();
                        else                               PS6_3_Clear();
                        if((bfr[length-8] & 0x08) >> 3)    PS6_4_Set();
                        else                               PS6_4_Clear();
                        if((bfr[length-8] & 0x10) >> 4)    PS6_5_Set();
                        else                               PS6_5_Clear();
                        if((bfr[length-8] & 0x20) >> 5)    PS6_6_Set();
                        else                               PS6_6_Clear();
                        if((bfr[length-7] & 0x01) >> 0)    PS7_1_Set();
                        else                               PS7_1_Clear();
                        if((bfr[length-7] & 0x02) >> 1)    PS7_2_Set();
                        else                               PS7_2_Clear();
                        if((bfr[length-7] & 0x04) >> 2)    PS7_3_Set();
                        else                               PS7_3_Clear();
                        if((bfr[length-7] & 0x08) >> 3)    PS7_4_Set();
                        else                               PS7_4_Clear();
                        if((bfr[length-7] & 0x10) >> 4)    PS7_5_Set();
                        else                               PS7_5_Clear();
                        if((bfr[length-7] & 0x20) >> 5)    PS7_6_Set();
                        else                               PS7_6_Clear();
                        if((bfr[length-6] & 0x01) >> 0)    PS8_1_Set();
                        else                               PS8_1_Clear();
                        if((bfr[length-6] & 0x02) >> 1)    PS8_2_Set();
                        else                               PS8_2_Clear();
                        if((bfr[length-6] & 0x04) >> 2)    PS8_3_Set();
                        else                               PS8_3_Clear();
                        if((bfr[length-6] & 0x08) >> 3)    PS8_4_Set();
                        else                               PS8_4_Clear();
                        if((bfr[length-6] & 0x10) >> 4)    PS8_5_Set();
                        else                               PS8_5_Clear();
                        if((bfr[length-6] & 0x20) >> 5)    PS8_6_Set();
                        else                               PS8_6_Clear();
                        if((bfr[length-5] & 0x01) >> 0)    EN_1_Set();
                        else                               EN_1_Clear();
                        if((bfr[length-5] & 0x02) >> 1)    EN_2_Set();
                        else                               EN_2_Clear();
                        if((bfr[length-5] & 0x04) >> 2)    EN_3_Set();
                        else                               EN_3_Clear();
                        if((bfr[length-5] & 0x08) >> 3)    EN_4_Set();
                        else                               EN_4_Clear();
                        if((bfr[length-5] & 0x10) >> 4)    EN_5_Set();
                        else                               EN_5_Clear();
                        if((bfr[length-5] & 0x20) >> 5)    EN_6_Set();
                        else                               EN_6_Clear();
                        if((bfr[length-5] & 0x40) >> 6)    EN_7_Set();
                        else                               EN_7_Clear();
                        if((bfr[length-5] & 0x80) >> 7)    EN_8_Set();
                        else                               EN_8_Clear();
                        if((bfr[length-4] & 0x01) >> 0)    SW_1_Set();
                        else                               SW_1_Clear();
                        if((bfr[length-4] & 0x02) >> 1)    SW_2_Set();
                        else                               SW_2_Clear();
                        if((bfr[length-4] & 0x04) >> 2)    SW_3_Set();
                        else                               SW_3_Clear();
                        if((bfr[length-4] & 0x08) >> 3)    SW_4_Set();
                        else                               SW_4_Clear();
                        if((bfr[length-4] & 0x10) >> 4)    SW_5_Set();
                        else                               SW_5_Clear();
                        if((bfr[length-4] & 0x20) >> 5)    SW_6_Set();
                        else                               SW_6_Clear();
                        if((bfr[length-4] & 0x40) >> 6)    SW_7_Set();
                        else                               SW_7_Clear();
                        if((bfr[length-4] & 0x80) >> 7)    SW_8_Set();
                        else                               SW_8_Clear();
                        
                        if((bfr[length-3] & 0x01) >> 0)    SW_9_Set();
                        else                               SW_9_Clear();
                        if((bfr[length-3] & 0x02) >> 1)    SSR_FAN_Set();
                        else                               SSR_FAN_Clear();
                        
                        memset(bfr, '\0', 30);                                                                                                                    // free up the buffer
                        send(appData.ClientSock[i], io_pack_ack_str, strlen(io_pack_ack_str), 0);
                        length = 0;                        
                    }
//                    send(appData.ClientSock[i], bfr, strlen(bfr), 0);
                }                 
                else if (length == 0) // || errno != EWOULDBLOCK) 
                {
                    closesocket(appData.ClientSock[i]);
                    appData.ClientSock[i] = INVALID_SOCKET;
                }
                // else just wait for some more data
            }
        }
            break;
        default:
            break;
    }
}
/*******************************************************************************
 End of File
 */
