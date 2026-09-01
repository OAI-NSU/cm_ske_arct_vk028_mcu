/**
 * @file mci.c
 * @author a-styuf (a-styuf@yandex.ru)
 * @brief блок для передачи данных через МЦИ
 * @version 0.1
 * @date 2026-09-04
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#include "mci.h"

void mci_init(typeMCIStruct *mci_ptr)
{
  mci_ptr->spi_inst[0] = SPI1;
  mci_ptr->spi_inst[1] = SPI3;

  /* PC5: SPI1_SCK,  PC6: SPI1_TX,  PC4: SPI1_FSS,  PC12: SPI3_SCK,  PC14: SPI3_TX,  PC15: SPI3_FSS */
  GPIOC->DENSET = (1<<15) | (1<<14) | (1<<12) | (1<<6) | (1<<5) | (1<<4);
  GPIOC->ALTFUNCSET = (1<<15) | (1<<14) | (1<<12) | (1<<6) | (1<<5) | (1<<4);
  GPIOC->ALTFUNCNUM0 |= (2<<6*4) | (2<<5*4) | (2<<4*4);
  GPIOC->ALTFUNCNUM1 |= (3<<(15-8)*4) | (3<<(14-8)*4) | (3<<(12-8)*4);
  /* PC13 -> SPI3_FSS,   PC2 -> SPI1_FSS */
  GPIOC->DATAOUTSET = (1<<13) | (1<<2);
  GPIOC->DENSET = (1<<13) | (1<<2);
  GPIOC->OUTENSET = (1<<13) | (1<<2);
  /**/
  GPIOC->INTTYPESET = (1<<12);
  GPIOC->INTENSET = (1<<12);
  /*GPIO filters*/
  GPIOC->QUALSET = (1<<12);
  GPIOC->QUALMODESET = (1<<12);
  /* SPI1 */
  mci_reset_parameters(mci_ptr, 0);
  /* SPI3 */
  mci_reset_parameters(mci_ptr, 1);
  /**/
  NVIC_EnableIRQ( SPI1_TX_IRQn );
  NVIC_EnableIRQ( SPI3_TX_IRQn );
}

void mci_reset_parameters(typeMCIStruct *mci_ptr, uint8_t spi_num)
{
  SPI_TypeDef *spi_ptr = mci_ptr->spi_inst[spi_num];
  uint32_t nss_port_msk;
  if(spi_num == 0)  {
    nss_port_msk = (1<<2);
    RCU->SPICFG[1].SPICFG_bit.RSTDIS = 0;
    RCU->SPICFG[1].SPICFG = RCU_SPICFG_SPICFG_DIVEN_Msk | (1<<8) | RCU_SPICFG_SPICFG_RSTDIS_Msk | RCU_SPICFG_SPICFG_CLKEN_Msk;
  }
  else {
    nss_port_msk = (1<<13);
    RCU->SPICFG[3].SPICFG_bit.RSTDIS = 0;
    RCU->SPICFG[3].SPICFG = RCU_SPICFG_SPICFG_DIVEN_Msk | (1<<8) | RCU_SPICFG_SPICFG_RSTDIS_Msk | RCU_SPICFG_SPICFG_CLKEN_Msk;
  }
  GPIOC->DATAOUTSET = nss_port_msk;

  mci_ptr->tx_ena[spi_num] = 0;
  mci_ptr->transaction_timeout[spi_num] = 0;
  mci_clear_fifo(mci_ptr);

  spi_ptr->CR0 = SPI_CR0_SPH_Msk | (8 - 1);  // 8 bit
  spi_ptr->CR1 = SPI_CR1_MS_Msk | SPI_CR1_SSE_Msk;  // Slave
  spi_ptr->IMSC_bit.TXIM = 1;
  GPIOC->DATAOUTCLR = nss_port_msk;
}

/**
  * @brief  обработчик планировщика задач
	* @param  mci_ptr указатель на структуру управления	
	* @param  time_us время мк us
  */
int8_t mci_process_tp(void* ctrl_struct, uint64_t time_us, typeProcessInterfaceStruct* interface)
{
	typeMCIStruct* mci_ptr = (typeMCIStruct*)ctrl_struct;
	if ((time_us - mci_ptr->last_call_time_us) > (MCI_PROCESS_PERIOD*1000)) {
		mci_ptr->call_interval_us = time_us - mci_ptr->last_call_time_us;
		mci_ptr->last_call_time_us = time_us;
		//
    for (uint8_t mci_num = 0; mci_num < 2; mci_num++){
      if (mci_ptr->tx_ena[mci_num]) mci_ptr->timeout_us[mci_num] += mci_ptr->call_interval_us;
      if (mci_ptr->timeout_us[mci_num] >= MCI_TRANS_TIMEOUT_MS*1000) {
        mci_ptr->timeout_us[mci_num] = 0;
        mci_ptr->tx_ena[mci_num] = 0;
        // блок подготовки новых данных
      }
    }
		//
		return 1;
	}
	else {
		return 0;
	}
}

void mci_clear_fifo(typeMCIStruct *mci_ptr)
{
  __NOP();
}


void SPIx_TX_IRQHandler(typeMCIStruct *mci_ptr, int spi_num) {
  SPI_TypeDef  *spi_ptr = mci_ptr->spi_inst[spi_num];
  if(spi_num == 0)   spi_ptr = (SPI_TypeDef*)SPI1_BASE;
  else               spi_ptr = (SPI_TypeDef*)SPI3_BASE;
  if(mci_ptr->tx_ena[spi_num]) mci_ptr->transaction_timeout[spi_num] = MCI_TRANS_TIMEOUT_MS;  // 6 ms timeout for gap
  while( spi_ptr->SR_bit.TNF ) {
    spi_ptr->DR = mci_ptr->buffer.buffer.array[mci_ptr->buffer.rd_ptr];
    if(mci_ptr->buffer.rd_ptr >= sizeof(type_MCI_FRAME_FIFO))  mci_ptr->buffer.rd_ptr = 0;
  }
  mci_ptr->tx_ena[spi_num] = 1;
}
