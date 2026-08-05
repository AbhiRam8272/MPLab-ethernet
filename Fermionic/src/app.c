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
#include <string.h>

/*
 * ORION beamformer add-on - ETHERNET ONLY, NO UART.
 *
 * If you delete UART1 / SYS_CONSOLE from MHC (you can - nothing in the ORION
 * path needs them), Harmony stops generating the SYS_CONSOLE_* macros and the
 * nine legacy calls left in YOUR code below would fail to compile. These
 * fallbacks make them no-ops so the project still builds either way.
 *
 * ORION itself never uses SYS_CONSOLE: it writes to an in-RAM boot log that is
 * replayed over TCP.
 */
#ifndef SYS_CONSOLE_MESSAGE
#define SYS_CONSOLE_MESSAGE(m)      do { (void)(m); } while (0)
#endif
#ifndef SYS_CONSOLE_PRINT
#define SYS_CONSOLE_PRINT(...)      do { } while (0)
#endif

#if ORION_ONLY 
#define ORION_BSD_SOCKETS_NEEDED (1 +MAX_CLIENT)
#else
#define ORION_BSD_SOCKETS_NEEDED ((1 +MAX_CLIENT) + (1 + ORON_MAX_CLIENT))
#endif

#if !defined(MAX_BSD_CLIENT)
//# error "MAX_BSD_SOCKETS undefined - enable the Berkeley API in MHC."
#elif MAX_BSD_SOCKETS < ORION_BSD_SOCKETS_NEEDED
# error "MAX_BSD_SOCKETS too small: port 2323 will fail with EMFILE .\
Set MHC -> TCP/IP Stack -> BSD API -> Max Sockets to 8 (need 1+MAX_CLIENT for \
port 5001 plus 1+ORION_MAX_CLINET for 2323) ."
#endif

#include "orion/orion.h"
#include "orion/orion_hal.h"
#include "orion/spi_hw.h"
#include "orion/console.h"
#include "orion/orion_board.h"

//#define SERVER_PORT 5001

#if ORION_ONLY
#define SERVER_PORT ORION_TCP_PORT  /* 2323 */
#else
#define SERVER_PORT 5001
#endif

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
// Section: ORION beamformer - state
// *****************************************************************************

static orion_t  g_orion;

/* Reply capture. console_putc() writes here while an ORION command runs. */
static char     s_txbuf[ORION_APP_TX_BUF];
static uint16_t s_txlen;
static bool     s_capturing;

extern uint8_t spi_hw_selftest(void);


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
// Section: ORION - console routing
// *****************************************************************************
/*
 * The shared ORION command interpreter only ever calls console_putc(). During
 * a network command the output is captured and flushed to the socket;
 * otherwise it is start-up output and goes to the boot log. That is what lets
 * ONE implementation of every command serve the socket.
 */

void console_putc(char c)
{
    if (s_capturing)
    {
        if (s_txlen < ORION_APP_TX_BUF - 1u)
        {
            s_txbuf[s_txlen++] = c;
        }
    }
    else
    {
        if (appData.bootlen < ORION_BOOT_LOG - 1u)
        {
            appData.bootlog[appData.bootlen++] = c;
        }
    }
}

/* No serial input path for ORION - TCP only. */
bool console_getc(char *c)
{
    (void) c;
    return false;
}

/* No console hardware to bring up. */
void console_hw_init(void)
{
}

// *****************************************************************************
// Section: ORION - socket helpers
// *****************************************************************************

static void orion_flush_reply(SOCKET s)
{
    int sent = 0;
    int n;

    if (s == INVALID_SOCKET || s_txlen == 0u)
    {
        s_txlen = 0;
        return;
    }

    while (sent < (int) s_txlen)
    {
        n = send(s, s_txbuf + sent, (int) s_txlen - sent, 0);
        if (n <= 0)
        {
            /* Peer window full. Drop the tail rather than spin here and
             * starve TCPIP_STACK_Task() - and your 5001 server with it. */
            break;
        }
        sent += n;
    }
    s_txlen = 0;
}

/*
 * Copy the boot log verbatim into the capture buffer.
 *
 * MUST NOT go through console_puts(): the log already holds CRLF (console_puts
 * expanded '\n' on the way in), so a second pass would emit CR CR LF and
 * render double-spaced.
 */
static void orion_emit_bootlog(void)
{
    uint16_t n;
    for (n = 0; n < appData.bootlen; n++)
    {
        if (s_txlen < ORION_APP_TX_BUF - 1u)
        {
            s_txbuf[s_txlen++] = appData.bootlog[n];
        }
    }
}

/* One received character from the ORION client. */
static void orion_feed(SOCKET s, char c)
{
    /*
     * While a bulk 'lut' transfer is running the parser is in raw hex mode:
     * incoming bytes are LUT payload, not command text. This MUST be checked
     * first or the hex stream is parsed as commands and silently does nothing.
     */
    if (cmd_lut_active())
    {
        s_txlen = 0; s_capturing = true;
        (void) cmd_lut_feed(c);
        s_capturing = false;
        if (s_txlen) orion_flush_reply(s);
        return;
    }

    if (c == '\r' || c == '\n')
    {
        if (appData.orionLen > 0u)
        {
            appData.orionLine[appData.orionLen] = '\0';
            s_txlen = 0; s_capturing = true;

            if (strcmp(appData.orionLine, "boot") == 0)
            {
                if (appData.bootlen) orion_emit_bootlog();
                else                 SYS_CONSOLE_MESSAGE("boot log empty\n");
            }
            else
            {
                cmd_execute(appData.orionLine);
                if (strcmp(appData.orionLine, "help") == 0)
                {
                    SYS_CONSOLE_MESSAGE("  boot                   replay start-up log\n");
                }
            }

            s_capturing = false;
            orion_flush_reply(s);
            appData.orionLen = 0;
        }

        /* If that command armed the LUT loader, do NOT print a prompt - the
         * next bytes are raw hex payload. */
        if (!cmd_lut_active())
        {
            s_txlen = 0; s_capturing = true;
            SYS_CONSOLE_MESSAGE("orion> ");
            s_capturing = false;
            orion_flush_reply(s);
        }
    }
    else if (c == 0x08 || c == 0x7F)            /* backspace / delete */
    {
        if (appData.orionLen) appData.orionLen--;
    }
    else if (c >= 0x20 && c < 0x7F)
    {
        if (appData.orionLen < CONSOLE_LINE_MAX - 1u)
        {
            appData.orionLine[appData.orionLen++] = c;
        }
    }
    /* Telnet IAC negotiation bytes (0xFF...) fall outside the printable
     * range and are discarded above. */
}

// *****************************************************************************
// Section: ORION - bring-up
// *****************************************************************************

static void orion_startup(void)
{
    orion_status_t st;
    uint8_t flags;
    uint8_t ch;

    SYS_CONSOLE_MESSAGE("\n=========================================\n"
                 " ORION beamformer on UDIGPSB0183\n"
                 " control: TCP port 2323\n"
                 "=========================================\n");

    spi_hw_init();
    orion_trx_set(ORION_MODE_RX);           /* safe idle: RX, PA off */
    orion_pa_enable(false);

    orion_init(&g_orion, ORION_SLV_DEFAULT);
    cmd_init(&g_orion);

    /* Let rails and the ORION internal POR settle before talking. */
    spi_hw_delay_us(50000);

    SYS_CONSOLE_MESSAGE("Probing beamformer...\n");
    st = orion_probe(&g_orion);

    SYS_CONSOLE_MESSAGE("  DEVICE_ID = 0x");
    console_print_hex8(g_orion.device_id);
    SYS_CONSOLE_MESSAGE("   REVISION = ");
    console_print_u16(g_orion.major_rev);
    console_putc('.');
    console_print_u16(g_orion.minor_rev);
    SYS_CONSOLE_MESSAGE("\n");

    if (st == ORION_OK)
    {
        SYS_CONSOLE_MESSAGE("  identification PASSED\n");

        if (g_orion.major_rev != ORION_MAJOR_REV_EXPECTED ||
            g_orion.minor_rev != ORION_MINOR_REV_EXPECTED)
        {
            SYS_CONSOLE_MESSAGE("  NOTE: revision differs from reference 1.1\n");
        }

        orion_rmw(&g_orion, REG_STG2_CFG, 0, 1, 1);     /* register-driven load */
        orion_rmw(&g_orion, REG_CORR_CFG, 0, 2, 0x3);   /* RX gain+phase corr   */

        for (ch = 0; ch < 4u; ch++)
        {
            orion_set_phase_tx(&g_orion, ch, 0);
            orion_set_gain_tx (&g_orion, ch, 0);
            orion_set_phase_rx(&g_orion, ch, 0);
            orion_set_gain_rx (&g_orion, ch, 0);
        }
        orion_update_code_pulse(&g_orion);
        SYS_CONSOLE_MESSAGE("  default configuration applied\n");
        return;
    }

    flags = spi_hw_selftest();
    if (flags & 0x02)
    {
        SYS_CONSOLE_MESSAGE("  NO DEVICE - MISO reads 0x00 on every register\n");
    }
    else if (flags & 0x01)
    {
        SYS_CONSOLE_MESSAGE("  MISO STUCK HIGH - reads 0xFF. Check power, MISO\n"
                     "  wiring, nCS reaching the device, level shifters.\n");
    }
    else
    {
        SYS_CONSOLE_MESSAGE("  WRONG DEVICE ID - bus toggles, answer is wrong.\n"
                     "  Suspect SPI mode (need CPOL=0 CPHA=0) or wiring.\n");
    }

    /* Deliberately NOT fatal - the server still starts so you can connect
     * and run 'probe' once the beamformer is powered. */
    SYS_CONSOLE_MESSAGE("  TCP server will still start - connect and 'probe'\n");
}

// *****************************************************************************
// Section: ORION - server, polled from APP_Tasks()
// *****************************************************************************
/*
 * Runs in the SAME state machine pass as your 5001 server. It is fully
 * non-blocking: Harmony 3 has no select()/poll(), so a blocking recv() here
 * would stall APP_Tasks() and starve both TCPIP_STACK_Task() and your own
 * phase-shifter protocol.
 *
 * recv() < 0 with errno == EWOULDBLOCK is the NORMAL idle case, not an error.
 */
//static void orion_server_tasks(void)
//{
//    struct sockaddr_in addRemote;
//    int  addrlen = sizeof (struct sockaddr_in);
//    char bfr[ORION_APP_RX_CHUNK];
//    int  length;
//    int  i, k;
//
//    /* Lazily create + bind + listen. Retried every pass until it succeeds. */
//    if (appData.orionServerSocket == INVALID_SOCKET)
//    {
//        struct sockaddr_in addr;
//        SOCKET skt = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
//        if (skt == INVALID_SOCKET)
//        {
//            return;
//        }
//
//        /* Harmony's bind() uses sin_port RAW - do NOT htons() it. Verified in
//         * berkeley_api.c: "lPort = local_addr->sin_port;" */
//        addr.sin_port = ORION_TCP_PORT;
//        addr.sin_addr.S_un.S_addr = IP_ADDR_ANY;
//
//        if (bind(skt, (struct sockaddr*) &addr, sizeof (addr)) == SOCKET_ERROR)
//        {
//            closesocket(skt);
//            return;
//        }
//        if (listen(skt, ORION_MAX_CLIENT) != 0)
//        {
//            closesocket(skt);
//            return;
//        }
//
//        appData.orionServerSocket = skt;
//
//        /* No UART: this goes to the boot log, replayed to the first client. */
//        s_capturing = false;
//        SYS_CONSOLE_MESSAGE("ORION server listening on port ");
//        console_print_u16((uint16_t) ORION_TCP_PORT);
//        SYS_CONSOLE_MESSAGE("\n");
//        return;
//    }
//
//    for (i = 0; i < ORION_MAX_CLIENT; i++)
//    {
//        if (appData.orionClientSock[i] == INVALID_SOCKET)
//        {
//            appData.orionClientSock[i] = accept(appData.orionServerSocket,
//                                                (struct sockaddr*) &addRemote,
//                                                &addrlen);
//            if (appData.orionClientSock[i] != INVALID_SOCKET)
//            {
//                appData.orionLen = 0;
//                s_txlen = 0; s_capturing = true;
//                cmd_banner();
//
//                /* Replay start-up diagnostics to the first client only -
//                 * nothing else has ever seen them. */
//                if (!appData.bootlog_sent && appData.bootlen)
//                {
//                    SYS_CONSOLE_MESSAGE("--- boot log ---\n");
//                    orion_emit_bootlog();
//                    SYS_CONSOLE_MESSAGE("--- end boot log ---\n");
//                    appData.bootlog_sent = true;
//                }
//
//                SYS_CONSOLE_MESSAGE("connected over TCP\n");
//                SYS_CONSOLE_MESSAGE("orion> ");
//                s_capturing = false;
//                orion_flush_reply(appData.orionClientSock[i]);
//            }
//        }
//
//        if (appData.orionClientSock[i] == INVALID_SOCKET)
//        {
//            continue;
//        }
//
//        length = recv(appData.orionClientSock[i], bfr, sizeof (bfr), 0);
//
//        if (length > 0)
//        {
//            for (k = 0; k < length; k++)
//            {
//                orion_feed(appData.orionClientSock[i], bfr[k]);
//            }
//        }
//        else if (length == 0 || errno != EWOULDBLOCK)
//        {
//            /* length == 0          : orderly close by the peer
//             * errno != EWOULDBLOCK : the connection is dead
//             * EWOULDBLOCK alone    : no data this pass - normal, do nothing */
//            closesocket(appData.orionClientSock[i]);
//            appData.orionClientSock[i] = INVALID_SOCKET;
//            appData.orionLen = 0;
//        }
//    }
//}

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

    /* --- ORION beamformer -------------------------------------------------
     * Sockets start invalid; orion_server_tasks() creates them once the
     * interface is up. The boot log must be cleared BEFORE orion_startup()
     * because that function writes straight into it. */
    {
        int oi;
        appData.orionServerSocket = INVALID_SOCKET;
        for (oi = 0; oi < ORION_MAX_CLIENT; oi++)
        {
            appData.orionClientSock[oi] = INVALID_SOCKET;
        }
        appData.orionLen     = 0;
        appData.bootlen      = 0;
        appData.bootlog_sent = false;
        s_txlen              = 0;
        s_capturing          = false;
    }

    orion_startup();
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
            int length, k = 0;
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
                
                appData.orionClientSock[i] = appData.ClientSock[i];
                length = recv(appData.orionClientSock[i], bfr, sizeof (bfr), 0);

                if (length > 0)
                {
                    for (k = 0; k < length; k++)
                    {
                        orion_feed(appData.orionClientSock[i], bfr[k]);
                    }
                }                
                
                // For all connected sockets, receive and send back the data
//                length = recv(appData.ClientSock[i], bfr, sizeof (bfr), 0);
//
//                if (length > 0) 
//                {
//                    SYS_CONSOLE_PRINT("%d\r\n", appData.ClientSock[i]);
//                    bfr[length] = '\0';
//                    if(bfr[length-4] == 'V' && bfr[length-3] == 'E' && bfr[length-2] == 'R' && bfr[length-1] == '?')                                // Version packet frame
//                    {
////                        ver_check_flag = 1;                                                                                                                                 // set this flag to give acknowledgment to client
//                        memset(bfr, '\0', 30);                                                                                                                    // free up the buffer
//                        send(appData.ClientSock[i], version_str, strlen(version_str), 0);
//                        length = 0;
//                    }      
////                    send(appData.ClientSock[i], bfr, strlen(bfr), 0);
//                }                 
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
