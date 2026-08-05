/*
 * console.h - UART2 operator console for the standalone ORION controller
 */

#ifndef CONSOLE_H
#define CONSOLE_H

#include <stdint.h>
#include <stdbool.h>
#include "orion.h"

#define CONSOLE_LINE_MAX    96

/* Transport (implemented by console_uart.c on the dsPIC, stubbed on host) */
void console_hw_init(void);
void console_putc(char c);
bool console_getc(char *c);         /* non-blocking; false if no byte ready */

/* Helpers */
void console_puts(const char *s);
void console_print_hex8(uint8_t v);
void console_print_u16(uint16_t v);

/* Command processing -------------------------------------------------------
 * cmd_execute() is pure logic (no UART access beyond console_puts), so the
 * whole command surface can be unit-tested on a host PC.
 */
void cmd_init(orion_t *dev);
void cmd_execute(char *line);       /* line is modified in place */
void cmd_banner(void);
void console_poll(void);            /* call from the main loop */

/*
 * Feed one character of the raw hex stream used by the bulk 'lut' command.
 * Returns true while the loader is active, meaning the character was consumed
 * and must NOT be treated as command-line input.
 *
 * console_poll() calls this for UART input. Any other transport (the TCP
 * command server) must call it too, or 'lut' will silently do nothing on that
 * transport.
 */
bool cmd_lut_feed(char c);
bool cmd_lut_active(void);

#endif /* CONSOLE_H */
