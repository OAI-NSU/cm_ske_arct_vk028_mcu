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
  RCU->HCLKCFG_bit.GPIOCEN = 1;
  RCU->HRSTCFG_bit.GPIOCEN = 1;
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
  //
  mci_clean_fifo(mci_ptr);
  mci_ptr->transaction_cnter[0] = 0;
  mci_ptr->transaction_cnter[1] = 0;
  mci_ptr->timeout_us[0] = 0;
  mci_ptr->timeout_us[1] = 0;
  mci_ptr->reinit_done[0] = 0;
  mci_ptr->reinit_done[1] = 0;
  mci_ptr->mci_need_to_clear[0] = 0;
  mci_ptr->mci_need_to_clear[1] = 0;
  memset((uint8_t*)mci_ptr->transaction_bit_cnter, 0x00, sizeof(mci_ptr->transaction_bit_cnter));
  memset((uint8_t*)mci_ptr->transaction_bit_cnter_num, 0x00, sizeof(mci_ptr->transaction_bit_cnter_num));
  /* SPI1 */
  mci_reinit_spi(mci_ptr, 0);
  /* SPI3 */
  mci_reinit_spi(mci_ptr, 1);
  // Таймер для МЦИ - ETMR0
  
  RCU->PCLKCFG0 |= RCU_PCLKCFG0_ETMR0EN_Msk;
  RCU->PRSTCFG0 |= RCU_PRSTCFG0_ETMR0EN_Msk;
  ETMR0->LOAD = ((uint64_t)(SystemCoreClock / (2 * 256)) * MCI_TRANS_STEP_MS) / 1000;

  ETMR0->CTRL_bit.PERMODE = 1;
  ETMR0->CTRL_bit.INTEN = 1;
  ETMR0->CTRL_bit.PRESC = 2;  //  0 - 1, 1 - 16, 2 - 256, 3 - RESERVE
  ETMR0->CTRL_bit.SIZE32 = 1;
  ETMR0->CTRL_bit.ONESH = 0;

  ETMR0->CTRL_bit.ON = 1;
  
  //
  mci_clean_buf(mci_ptr, 0);
  mci_clean_buf(mci_ptr, 1);
  /**/
  NVIC_EnableIRQ( SPI1_TX_IRQn );
  NVIC_EnableIRQ( SPI3_TX_IRQn );
  NVIC_EnableIRQ( ETMR0_IRQn );
}

void mci_reinit_spi(typeMCIStruct *mci_ptr, uint8_t spi_num)
{
  SPI_TypeDef *spi_ptr = mci_ptr->spi_inst[spi_num];
  uint32_t nss_port_msk;

  if(spi_num == 0)  {
    nss_port_msk = (1<<2);

    RCU->SPICFG[1].SPICFG_bit.RSTDIS = 0;
    for(uint32_t delay = 1; delay>0; delay--){
      __NOP();
    }
    RCU->SPICFG[1].SPICFG = RCU_SPICFG_SPICFG_DIVEN_Msk | (1<<8) | RCU_SPICFG_SPICFG_RSTDIS_Msk | RCU_SPICFG_SPICFG_CLKEN_Msk;
  }
  else {
    nss_port_msk = (1<<13);

    RCU->SPICFG[3].SPICFG_bit.RSTDIS = 0;
    for(uint32_t delay = 1; delay>0; delay--){
      __NOP();
    }
    RCU->SPICFG[3].SPICFG = RCU_SPICFG_SPICFG_DIVEN_Msk | (1<<8) | RCU_SPICFG_SPICFG_RSTDIS_Msk | RCU_SPICFG_SPICFG_CLKEN_Msk;
  }
  GPIOC->DATAOUTSET = nss_port_msk;

  mci_ptr->tx_ena[spi_num] = 0;
  mci_ptr->timeout_us[spi_num] = 0;

  spi_ptr->CR0 = SPI_CR0_SPH_Msk | (8 - 1);  // 8 bit
  spi_ptr->CR1 = SPI_CR1_MS_Msk | SPI_CR1_SSE_Msk;  // Slave
  spi_ptr->IMSC_bit.TXIM = 1;
  GPIOC->DATAOUTCLR = nss_port_msk;
	//
	mci_ptr->reinit_done[spi_num] = 1;
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
		return 1;
	}
	else {
		return 0;
	}
}

uint8_t mci_tx_preparation(typeMCIStruct *mci_ptr, uint64_t interval_us)
{
  //
  for (uint8_t mci_num = 0; mci_num < 2; mci_num++){
    if (mci_ptr->tx_ena[mci_num]) {
      mci_ptr->timeout_us[mci_num] += interval_us;
    }
    else if ((mci_ptr->tx_ena[mci_num] == 0) && (mci_ptr->timeout_us[mci_num] != 0)) {
      mci_ptr->timeout_us[mci_num] = 0;
    }
    if ((mci_ptr->tx_ena[mci_num]) && (mci_ptr->timeout_us[mci_num] >= MCI_TRANS_TIMEOUT_MS*1000)) {
      if (mci_num == 0) NVIC_DisableIRQ(SPI1_TX_IRQn);
      else if (mci_num == 1) NVIC_DisableIRQ(SPI3_TX_IRQn);
      NVIC_DisableIRQ( ETMR0_IRQn );
      mci_ptr->timeout_us[mci_num] = 0;
      mci_ptr->tx_ena[mci_num] = 0;
      mci_ptr->transaction_bit_cnter_num[mci_num] = (++mci_ptr->transaction_bit_cnter_num[mci_num]) % 8;
      mci_ptr->transaction_bit_cnter[mci_num][mci_ptr->transaction_bit_cnter_num[mci_num]] = 0;
      mci_ptr->transaction_cnter[mci_num]++;
      if (mci_ptr->mci_need_to_clear[mci_num]){
        mci_ptr->mci_need_to_clear[mci_num] = 0;
        mci_clean_buf(mci_ptr, mci_num);
        mci_fill_buf(mci_ptr, mci_num);
      }
      mci_reinit_spi(mci_ptr, mci_num);
      //
      NVIC_EnableIRQ( SPI1_TX_IRQn );
      NVIC_EnableIRQ( SPI3_TX_IRQn );
      NVIC_EnableIRQ( ETMR0_IRQn );
    }
  } 
	return 0;
}

/**
 * @brief Заполнение кадрами массива информации для МЦИ либо заполнителем (0xFE)
 * 
 * @param mci_ptr 
 */
void mci_fill_buf(typeMCIStruct *mci_ptr, uint8_t spi_num)
{
  uint8_t frame[sizeof(typeFrameStruct)] = {0};
  uint8_t header_buff[6] = MCI_HEADER_LABEL;

  memset((uint8_t*)mci_ptr->buffer[spi_num].buffer.array, 0xFE, sizeof(type_MCI_FRAME_BUFFER));

  memcpy(&mci_ptr->buffer[spi_num].buffer.fd.header.mci_label[0], header_buff, sizeof(header_buff));
  mci_ptr->buffer[spi_num].buffer.fd.header.time_s = clock_get_time_s();
  
  for (uint8_t frame_num=0; frame_num < MCI_BUFF_FRAMES; frame_num++){
    if (mci_read_fifo(mci_ptr,  frame)){
      memcpy((uint8_t*)&mci_ptr->buffer[spi_num].buffer.fd.frame[frame_num], frame, sizeof(typeFrameStruct));
    }
    else{
      break;
    }
  }
}

void mci_clean_buf(typeMCIStruct *mci_ptr, uint8_t spi_num)
{
  memset((uint8_t*)&mci_ptr->buffer[spi_num].buffer, 0xFE, sizeof(type_MCI_FRAME_BUFFER));
  mci_ptr->buffer[spi_num].rd_ptr = 0;
}


void mci_clean_fifo(typeMCIStruct *mci_ptr)
{
  memset((uint8_t*)&mci_ptr->fifo, 0x00, sizeof(type_MCI_Frame_FIFO));
}

int8_t mci_write_fifo(typeMCIStruct *mci_ptr, uint8_t *frame)
{
  if (mci_ptr->fifo.rec_num >= MCI_FIFO_DEPTH){
		return 0;
	}
	else{
		memcpy((uint8_t*)&mci_ptr->fifo.rec_fifo[mci_ptr->fifo.rec_num], (uint8_t*)frame, sizeof(typeFrameStruct));
		mci_ptr->fifo.rec_num += 1;
		if (mci_ptr->fifo.rec_num > mci_ptr->fifo.rec_max) mci_ptr->fifo.rec_max = mci_ptr->fifo.rec_num;  //сбор информации о заполненности fifo
		return 1;
	}
}

int8_t mci_read_fifo(typeMCIStruct *mci_ptr, uint8_t *frame)
{
  if (mci_ptr->fifo.rec_num == 0){
		return 0;
	}
	else{
		mci_ptr->fifo.rec_num -= 1;
		memcpy((uint8_t*)frame, (uint8_t*)&mci_ptr->fifo.rec_fifo[0], sizeof(typeFrameStruct));
		memmove((uint8_t*)&mci_ptr->fifo.rec_fifo[0], (uint8_t*)&mci_ptr->fifo.rec_fifo[1], sizeof(typeFrameStruct)*mci_ptr->fifo.rec_num);
		memset((uint8_t*)&mci_ptr->fifo.rec_fifo[mci_ptr->fifo.rec_num], 0x00, sizeof(typeFrameStruct)*(MCI_FIFO_DEPTH - mci_ptr->fifo.rec_num));
		return 1;
	}
}

uint8_t mci_get_fifo_state(typeMCIStruct *mci_ptr)
{
  if (mci_ptr->fifo.rec_num >= MCI_FIFO_DEPTH) return 0;
  return 1;
}


void mci_spi_tx_irq_handler(typeMCIStruct *mci_ptr, int spi_num) 
{
  SPI_TypeDef  *spi_ptr = mci_ptr->spi_inst[spi_num];
  if(mci_ptr->tx_ena[spi_num]) mci_ptr->timeout_us[spi_num] = 0;  // 6 ms timeout for gap
  while( spi_ptr->SR_bit.TNF ) {
    spi_ptr->DR = mci_ptr->buffer[spi_num].buffer.array[mci_ptr->buffer[spi_num].rd_ptr++];
    mci_ptr->transaction_bit_cnter[spi_num][mci_ptr->transaction_bit_cnter_num[spi_num]]++;
    if(mci_ptr->buffer[spi_num].rd_ptr >= sizeof(type_MCI_FRAME_BUFFER))  {
			mci_ptr->buffer[spi_num].rd_ptr = 0;
		}
    if (mci_ptr->buffer[spi_num].rd_ptr >= 6){
      mci_ptr->mci_need_to_clear[spi_num] = 1;
    }
  }
	//
	if (mci_ptr->reinit_done[spi_num]) {
		mci_ptr->reinit_done[spi_num] = 0;
	}
	else{
		mci_ptr->tx_ena[spi_num] = 1;
	}
}

