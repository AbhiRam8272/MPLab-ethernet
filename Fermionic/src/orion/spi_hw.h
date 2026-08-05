/*
 * spi_hw.h - thin SPI1 + control-line abstraction for the ORION driver
 *
 * Implemented by spi_hw.c on the dsPIC. A host-side stub implements the same
 * three functions so the driver logic can be unit-tested on a PC.
 */

#ifndef SPI_HW_H
#define SPI_HW_H

#include <stdint.h>
#include <stdbool.h>

void spi_hw_init(void);

/*
 * Full-duplex burst with chip select asserted for the whole transfer.
 *   tx  : bytes to clock out (never NULL)
 *   rx  : buffer for bytes clocked in, or NULL to discard
 *   len : number of bytes
 */
void spi_hw_xfer(const uint8_t *tx, uint8_t *rx, uint16_t len);

/* Control lines (formerly the bridge's I2C GPIO expander) */
void spi_hw_set_tr(bool level);
void spi_hw_set_pa(bool level);
void spi_hw_set_txl(bool level);
void spi_hw_set_rxl(bool level);

/* Short blocking delay used for strobe widths */
void spi_hw_delay_us(uint16_t us);

#endif /* SPI_HW_H */
