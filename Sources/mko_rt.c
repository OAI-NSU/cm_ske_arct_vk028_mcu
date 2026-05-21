/**
 * @file mko_rt.c
 * @author your name (you@domain.com)
 * @brief управление МПИ УТ для 1928ВК028
 * @version 0.1
 * @date 2024-02-12
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#include "mko_rt.h"


volatile typeMKORT_SubaddrTbl  	MKORT_SubaddrTbl[32]  	  __attribute__((aligned (32*16)));
volatile typeMKORT_Desc  		    MKORT_Desc[32] 			      __attribute__((aligned (32*16)));
volatile typeMKORT_Log 			    MKORT_Log 				        __attribute__((aligned (4)));
volatile uint16_t  				      MKORT_Buff[32*32]         __attribute__((aligned (32*16)));

/**
  * @brief  инициализация программной модели МКО
  * @param  mko_ptr указатель на программную модель устройства 
  * @param  mko_addr 0 - значение берется с перемычек, не 0 - используется указанное значение (только для MKO_RT)
  * @return int8_t 1 - успешная инициализация, 0 - ошибка адреса
  */
int8_t mko_rt_init(typeMKORTStruct *mko_rt_ptr, uint8_t mko_addr) 
{
  //
  printf("%s: mko_rt init start \n",  now());
  //
  mko_rt_ptr->addr = (mko_addr == 0) ? mko_rt_get_addr_from_gpio(mko_rt_ptr) : mko_addr;
  mko_rt_ptr->error = 0x00;
  mko_rt_ptr->error_cnt = 0x00;
  mko_rt_ptr->sa_rx = 0;
  memset((uint8_t*)mko_rt_ptr->sa_rx_data, 0x00, sizeof(64));
  mko_rt_ptr->rx_cnt = 0;
  mko_rt_ptr->tx_cnt = 0;
  mko_rt_ptr->cmd_cnt = 0;
  mko_rt_ptr->irq_cnter = 0;
  //
  mko_rt_ptr->inst = (MILSTD_TypeDef_Simple*)MILSTD0;
  //
  // mko_rt_set_busy(mko_rt_ptr);
  // аппаратная настройка ядра МПИ
  int n;
  //
  RCU->HRSTCFG |= RCU_HRSTCFG_GPIOJEN_Msk;  RCU->HCLKCFG |= RCU_HCLKCFG_GPIOJEN_Msk;
  //
  GPIOJ->DENSET = (0xFF << 6);
  GPIOJ->ALTFUNCSET = (0xFF << 6);
  GPIOJ->ALTFUNCNUM0 |= 0x11000000;
  GPIOJ->ALTFUNCNUM1 |= 0x00111111;
  RCU->MILSTDCFG[0].MILSTDCFG_bit.DIVN = 1;   // 80 / 4 = 20 MHz
  RCU->MILSTDCFG[0].MILSTDCFG_bit.CLKSEL = 1;  // from PLLCLK
  RCU->MILSTDCFG[0].MILSTDCFG |= RCU_MILSTDCFG_MILSTDCFG_DIVEN_Msk | RCU_MILSTDCFG_MILSTDCFG_RSTDIS_Msk | RCU_MILSTDCFG_MILSTDCFG_CLKEN_Msk;
  RCU->HRSTCFG_bit.MILSTD0EN = 1;
  RCU->HCLKCFG_bit.MILSTD0EN = 1;
  //
  MILSTD0->IENR_bit.RTEVE |= 1;
  /*Descriptors fill*/
	mko_rt_ptr->MKORT_Log_ptr = &MKORT_Log;
	mko_rt_ptr->MKORT_Buff = MKORT_Buff;
	mko_rt_ptr->MKORT_Desc = MKORT_Desc;
	mko_rt_ptr->MKORT_SubaddrTbl = 	MKORT_SubaddrTbl;
  //
  for(n=0; n<32; n++) {
    switch(n){
      default:
        MKORT_SubaddrTbl[n].CtrlWord.igndv = 1;
        MKORT_SubaddrTbl[n].CtrlWord.rxen = 1;
        MKORT_SubaddrTbl[n].CtrlWord.rxlog = 1;
				MKORT_SubaddrTbl[n].CtrlWord.rxirq = 1;
        MKORT_SubaddrTbl[n].CtrlWord.txen = 1;
        MKORT_SubaddrTbl[n].CtrlWord.txlog = 1;
        MKORT_SubaddrTbl[n].DescRx = &MKORT_Desc[n];
        MKORT_SubaddrTbl[n].DescTx = &MKORT_Desc[n];
        MKORT_Desc[n].BuffAddr = (uint32_t)&MKORT_Buff[32*n];
        MKORT_Desc[n].NextPtr = (uint32_t)&MKORT_Desc[n];
      break;
    }
  }
  // управление командами УТ
  MILSTD0->RTMOD_bit.TS = 0x2;
  MILSTD0->RTMOD_bit.TSB = 0x2;
  MILSTD0->RTMOD_bit.RRT = 0x2;
  MILSTD0->RTMOD_bit.RRTB = 0x2;
  //
  MILSTD0->RTELP = (uint32_t)&MKORT_Log;
  MILSTD0->RTSADDR = (uint32_t)MKORT_SubaddrTbl;
  //
  if (mko_rt_ptr->addr != 0){
    MILSTD0->RTCON = (0x1553<<16) | ((mko_rt_ptr->addr & 0x1F) << 1) | MILSTD_RTCON_RTEN_Msk;
  }
  else{
    //TODO: разобраться как привязать адрес из ядра МПИ и заменить прямое чтение с GPIO
    printf("%s: !!! mko_rt init finish with error: incorrect address \n",  now());
    return 0;
  }
  //
  // mko_rt_release_busy(mko_rt_ptr);
  mko_rt_rx_fifo_init(mko_rt_ptr);
  printf("%s: mko_rt init finish, address <%d> \n",  now(), mko_rt_ptr->addr);
  NVIC_EnableIRQ(MILSTD0_IRQn);
  //
  return 1;
}

/**
 * @brief Установка состояния "Занят"
 * 
 * @param mko_rt_ptr 
 */
void mko_rt_set_busy(typeMKORTStruct *mko_rt_ptr)
{
  MILSTD0->RTBST_bit.BUSY = 1;
}

/**
 * @brief Снятие состояния занят
 * 
 * @param mko_rt_ptr 
 */
void mko_rt_release_busy(typeMKORTStruct *mko_rt_ptr)
{
  MILSTD0->RTBST_bit.BUSY = 0;
}

/**
 * @brief NU, не реализуемо на данном МК
 * 
 * @param mko_rt_ptr 
 * @param val 
 */
void mko_rt_set_aw_bit_7(typeMKORTStruct *mko_rt_ptr, uint8_t val)
{
  //
}

/**
 * @brief установка нулевых значений в ПА МПИ
 * 
 * @param mko_rt_ptr 
 */
void mko_rt_clear_data(typeMKORTStruct *mko_rt_ptr)
{
  mko_rt_set_busy(mko_rt_ptr);
  memset((uint8_t*)&mko_rt_ptr->MKORT_Buff[32*32], 0x00, sizeof(MKORT_Buff));
  mko_rt_release_busy(mko_rt_ptr);
}

/**
 * @brief обработка принятых сообщений МКО
 * 
 * @param mko_rt_ptr 
 * @return uint8_t возвращает 1 при записи SA, 2 при чтении SA, 3 - команда,  либо 0 - нет транзакции
 */
uint8_t mko_rt_transaction_handler(typeMKORTStruct *mko_rt_ptr, uint8_t* sa_ptr)
{
  uint8_t retval = 0;
  typeMPI_RT_Fifo_Data rx_data;
  uint8_t rx_fifo_status = 0;
  mko_rt_ptr->log_msg = *mko_rt_ptr->MKORT_Log_ptr;
	MKORT_Log.u32 = 0;
  //
  rx_fifo_status = mko_rt_read_rx_fifo(mko_rt_ptr, &rx_data);
  //
  if(rx_fifo_status){
    mko_rt_ptr->sa_rx = rx_data.log.uf.samc & 0x1F;
		*sa_ptr = mko_rt_ptr->sa_rx;
    mko_rt_ptr->error = mko_rt_ptr->log_msg.uf.tres;
    memcpy((uint8_t*)mko_rt_ptr->sa_rx_data, (uint8_t*)rx_data.data, 32);
    retval = 1;
  }
  else if (mko_rt_ptr->log_msg.u32 == 0){
    retval = 0;
  }
  else {
    mko_rt_ptr->error = mko_rt_ptr->log_msg.uf.tres;
    switch(mko_rt_ptr->log_msg.uf.tres){
      case(TRES_SUCCESS):
        switch(mko_rt_ptr->log_msg.uf.type){
          case(TYPE_RX_DATA):
            mko_rt_ptr->tx_cnt++;
            //
            mko_rt_ptr->sa_rx = mko_rt_ptr->log_msg.uf.samc & 0x1F;
            *sa_ptr = mko_rt_ptr->sa_rx;
            //
            retval = 2;
            break;
          case(TYPE_TX_DATA): //обработка перенесена в прерывание
            // mko_rt_ptr->rx_cnt++;
            // //
            // mko_rt_ptr->sa_rx = mko_rt_ptr->log_msg.uf.samc & 0x1F;
            // mko_rt_read_from_subaddr(mko_rt_ptr, mko_rt_ptr->sa_rx, mko_rt_ptr->sa_rx_data);
            // *sa_ptr = mko_rt_ptr->sa_rx;
            // //
            // retval = 1;
            break;
          case(TYPE_CMD):
            mko_rt_ptr->cmd_cnt++;
            //
            __mko_rt_cmd_msg(mko_rt_ptr, mko_rt_ptr->log_msg.uf.samc);
            //
            retval = 3;
          break;
        }
      break;
      default:
        mko_rt_ptr->error_cnt++;
      break;
    }
  }
  return retval;
}

void mko_rt_write_to_subaddr(typeMKORTStruct *mko_rt_ptr, uint8_t subaddr, uint16_t* data)
{
  mko_rt_set_busy(mko_rt_ptr);
  //
  memcpy((uint8_t*)&mko_rt_ptr->MKORT_Buff[32 * (subaddr & 0x1F)], (uint8_t*)data, 64);
  //
  mko_rt_release_busy(mko_rt_ptr);
}

void mko_rt_read_from_subaddr(typeMKORTStruct *mko_rt_ptr, uint8_t subaddr, uint16_t* data)
{
  mko_rt_set_busy(mko_rt_ptr);
  //
  memcpy((uint8_t*)data, (uint8_t*)&mko_rt_ptr->MKORT_Buff[32 * (subaddr & 0x1F)], 64);
  //
  mko_rt_release_busy(mko_rt_ptr);
}


/**
 * @brief Инициализация fifo-приема
 * 
 * @param mko_rt_ptr 
 * @return int8_t 
 */
int8_t mko_rt_rx_fifo_init(typeMKORTStruct* mko_rt_ptr)
{
	memset((uint8_t*)&mko_rt_ptr->rx_fifo, 0x00, sizeof(typeMPIRT_RX_Fifo));
	return 0;
}

/**
 * @brief запись данных в fifo для принятых данных
 * @param mko_rt_ptr 
 * @param data 
 * @return int8_t 1 - ОК, 0 - записано, но с потерей пакета из-за переполнения
 */
int8_t mko_rt_write_rx_fifo(typeMKORTStruct* mko_rt_ptr, typeMPI_RT_Fifo_Data* data)
{
	uint8_t ret_val = 1;
	memcpy((uint8_t*)&mko_rt_ptr->rx_fifo.array[mko_rt_ptr->rx_fifo.wr_ptr], (uint8_t*)data, sizeof(typeMPI_RT_Fifo_Data));
	mko_rt_ptr->rx_fifo.rec_full++;
	//
	if((++mko_rt_ptr->rx_fifo.wr_ptr) >= 8) mko_rt_ptr->rx_fifo.wr_ptr = 0;
	//
	if ((mko_rt_ptr->rx_fifo.wr_ptr == mko_rt_ptr->rx_fifo.rd_ptr)){
		if(++mko_rt_ptr->rx_fifo.rd_ptr >= 8) mko_rt_ptr->rx_fifo.rd_ptr = 0;
		mko_rt_ptr->rx_fifo.rec_lost++;
		ret_val = 0;
	}
	//
	mko_rt_rx_fifo_rec_num_update(mko_rt_ptr);
	return ret_val;
}

/**
 * @brief чтение данных из fifo для принятых данных
 * 
 * @param mko_rt_ptr 
 * @param data 
 * @return int8_t 1 - ОК, 0 - нет данных
 */
int8_t mko_rt_read_rx_fifo(typeMKORTStruct* mko_rt_ptr, typeMPI_RT_Fifo_Data* data)
{
	if(mko_rt_ptr->rx_fifo.rec_num){
		memcpy((uint8_t *)data, (uint8_t*)&mko_rt_ptr->rx_fifo.array[mko_rt_ptr->rx_fifo.rd_ptr], sizeof(typeMPI_RT_Fifo_Data));
    memcpy((uint8_t*)&mko_rt_ptr->rx_fifo.last_rec, (uint8_t*)&mko_rt_ptr->rx_fifo.array[mko_rt_ptr->rx_fifo.rd_ptr], sizeof(typeMPI_RT_Fifo_Data));
		if(++mko_rt_ptr->rx_fifo.rd_ptr >= 8) mko_rt_ptr->rx_fifo.rd_ptr = 0;
		mko_rt_rx_fifo_rec_num_update(mko_rt_ptr);
		return 1;
	}
	else{
		return 0;
	}
}

/**
 * @brief обновление количества записей в FIFO по состояниям указателя чтения и записи.
 * 
 * @param mko_rt_ptr 
 */
void mko_rt_rx_fifo_rec_num_update(typeMKORTStruct* mko_rt_ptr)
{
	mko_rt_ptr->rx_fifo.rec_num = (mko_rt_ptr->rx_fifo.wr_ptr >= mko_rt_ptr->rx_fifo.rd_ptr) ? (mko_rt_ptr->rx_fifo.wr_ptr - mko_rt_ptr->rx_fifo.rd_ptr) 
																					: (8 - (mko_rt_ptr->rx_fifo.rd_ptr - mko_rt_ptr->rx_fifo.wr_ptr));
	mko_rt_ptr->rx_fifo.rec_num_max = (mko_rt_ptr->rx_fifo.rec_num > mko_rt_ptr->rx_fifo.rec_num_max) ? mko_rt_ptr->rx_fifo.rec_num : mko_rt_ptr->rx_fifo.rec_num_max;
}

/**
 * @brief чтение адреcов МКО с GPIO
 * 
 * @param mko_rt_ptr 
 * @return uint8_t 0 - ошибка, не 0 - адрес
 */
uint8_t mko_rt_get_addr_from_gpio(typeMKORTStruct *mko_rt_ptr)
{
  /*get mko address*/
  uint8_t mko_psum = 0; 
  uint8_t mko_addr = 0; 
  //
	RCU->HRSTCFG |= RCU_HRSTCFG_GPIOJEN_Msk;  RCU->HCLKCFG |= RCU_HCLKCFG_GPIOJEN_Msk;
  GPIOJ->DENSET = 0x3F;
  GPIOJ->PULLMODE |= 0x555;
  while ((GPIOJ->PULLMODE & 0x555) != 0x555) {};
  mko_addr = GPIOJ->DATA & 0x3F;
  for(uint8_t i=0; i<6; i++) {
    mko_psum = mko_psum ^ ((mko_addr >> i) & 1);
  }
  mko_addr = (mko_addr >> 1) & 0x1F;
  if((mko_addr == 0x1F)||(mko_addr == 0)||(mko_psum == 0)){
    return 0;
  }
  return mko_addr;
}

/**
  * @brief  обработка командных сообщений согласно протоколу МКО
  * @note  в случае приема командного слова длина выступает кодом команды
  * @param  mko_ке_ptr указатель на программную модель устройства
  */
void __mko_rt_cmd_msg(typeMKORTStruct *mko_rt_ptr, uint8_t cmd_num)
{
  // printf("mko cmd msg %d\n", mko_ptr->cw.field.leng);
  switch (cmd_num){
    case 2: //передать ответное слово
      // ничего не делаем, ответное слово передается ядром
      break;
    case 4: //блокировать передатчик
      // реализована аппаратно
      break;
    case 5: //разблокировать передатчик
      // реализована аппаратно
      break;
    case 8: //разблокировать передатчик
      // реализована аппаратно
      break;
  }
}

/**
  * @brief  возврат значения ошибок модуля МКО
  * @param  mko_ptr указатель на программную модель устройства
	* @param  error указатель на переменную с ошибкой МКО
	* @param  error_cnt указатель на переменную с счетчиком ошибок МКО
  */
void mko_rt_get_error(typeMKORTStruct *mko_rt_ptr, uint8_t* error, uint8_t* error_cnt)
{
	*error = mko_rt_ptr->error;
	*error_cnt = mko_rt_ptr->error_cnt;
}

void mko_rt_irq_rx_callback(typeMKORTStruct *mko_rt_ptr)
{
  typeMPI_RT_Fifo_Data data;
  uint16_t sa_rx;
	__disable_irq();
	if (MILSTD0->IR_bit.RTEV){

    data.log = *mko_rt_ptr->MKORT_Log_ptr;
    //
    mko_rt_ptr->rx_cnt++;
    sa_rx = data.log.uf.samc & 0x1F;
    mko_rt_read_from_subaddr(mko_rt_ptr, sa_rx, data.data);
		if (memcmp((uint8_t*)data.data, (uint8_t*)&mko_rt_ptr->MKORT_Buff[32 * (sa_rx & 0x1F)], 64) == 0){
      //
		}
		else{
			__BKPT(0);
		}
    mko_rt_write_rx_fifo(mko_rt_ptr, &data);
    //
  }
  MILSTD0->IR = MILSTD0->IR;
	__enable_irq();
}
