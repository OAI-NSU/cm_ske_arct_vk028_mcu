#include "spi.h"

void SPI_Init(void) {
  /* ~CS */
  // GPIOA->DENSET = (1<<8);
  // GPIOA->OUTENSET = (1<<8);
  /* GPIO */
  RCU->HRSTCFG |= RCU_HRSTCFG_GPIOAEN_Msk;  RCU->HCLKCFG |= RCU_HCLKCFG_GPIOAEN_Msk;
  //
  GPIOA->DENSET = 0x07;
  GPIOA->PULLMODE |= (1<<1*2);
  GPIOA->ALTFUNCSET = 0x07;
  GPIOA->ALTFUNCNUM0 |= (1<<2*4) | (1<<1*4) | (1<<0*4);
  /* SPI */
  RCU->SPICFG[0].SPICFG = RCU_SPICFG_SPICFG_DIVEN_Msk | (1<<8) | RCU_SPICFG_SPICFG_RSTDIS_Msk | RCU_SPICFG_SPICFG_CLKEN_Msk;
	//
  SPI0->CR0 = 8 - 1;  // 8 bit
  SPI0->CPSR = 4;  // 50 MHz / 4 = 12,5 MHz
  SPI0->CR1 = SPI_CR1_SSE_Msk;
}

uint8_t spi_exchange(uint8_t data) {
  SPI0->DR = data;
  while((SPI0->SR & SPI_SR_BSY_Msk) != 0);  //while busy
  return (uint8_t)SPI0->DR;
}

void spi_mexchange(uint8_t *data_in, uint8_t *data_out, int len) {
  int i;
  if(data_in == NULL) {
    for(i=0; i<len; i++) spi_exchange(data_out[i]);
    }
  else if(data_out == NULL) {
    for(i=0; i<len; i++) data_in[i] = spi_exchange(0);
    }
  else {
    for(i=0; i<len; i++) data_in[i] = spi_exchange(data_out[i]);
    }
}

void SPI_Exchange(uint8_t data)
{
  spi_exchange(data);
}

/**
 * @brief запись/чтение в SPI. Создана для совместимости с прошлыми проектами.
 * 
 * @param data_in 
 * @param data_out 
 * @param len 
 */
void SPI_MExchange(uint8_t *data_in, uint8_t *data_out, int len)
{
  spi_mexchange(data_in, data_out, len);
}


