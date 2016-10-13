
#ifndef ARM_SPI_H
#define ARM_SPI_H

#include "stdint.h"

#define SPI_TIMEOUT 15000

// Functions
void SPI_init(void);
uint16_t SPI_write(uint16_t data, uint8_t lastxfer);

#endif // ARM_SPI_H_