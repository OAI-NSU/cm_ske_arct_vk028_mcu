#ifndef _SPI_H_
#define _SPI_H_

#include "CM4/K1921VK028.h"
#include "system_K1921VK028.h"

#ifndef NULL
#define NULL ((void*)0)
#endif

void SPI_Init(void);
void SPI_Exchange(uint8_t data);
void SPI_MExchange(uint8_t *data_in, uint8_t *data_out, int len);

#endif
