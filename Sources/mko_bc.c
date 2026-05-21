/**
 * @file mko_bc.c
 * @author your name (you@domain.com)
 * @brief управление МПИ КШ для 1928ВК028
 * @version 0.1
 * @date 2024-02-12
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#include "mko_bc.h"

volatile typeMKOBC_DescList MKOBC_DescListRx __attribute__((aligned (16)));
volatile typeMKOBC_DescList MKOBC_DescListTx __attribute__((aligned (16)));

uint16_t MKOIVect;

/**
  * @brief  инициализация программной модели МКО
  * @param  mko_ptr указатель на программную модель устройства 
  * @param  type тип устройства (MKO_BC или MKO_RT)
  * @param  regs указатель на регистры управления МКО
  * @param  mko_addr 0 - значение берется с перемычек, не 0 - используется указанное значение (только для MKO_RT)
  */
void mko_bc_init(typeMKOBCStruct *mko_ptr) 
{
  // аппаратная настройка ядра МПИ
  RCU->HRSTCFG |= RCU_HRSTCFG_GPIOKEN_Msk;  RCU->HCLKCFG |= RCU_HCLKCFG_GPIOKEN_Msk;
  //
  GPIOK->DENSET = 0xFF;
  GPIOK->ALTFUNCSET = 0xFF;
  GPIOK->ALTFUNCNUM0 |= 0x11111111;
  RCU->MILSTDCFG[1].MILSTDCFG_bit.DIVN = 1;   // 80 / 4 = 20 MHz
  RCU->MILSTDCFG[1].MILSTDCFG_bit.CLKSEL = 1;  // from PLLCLK
  RCU->MILSTDCFG[1].MILSTDCFG |= RCU_MILSTDCFG_MILSTDCFG_DIVEN_Msk | RCU_MILSTDCFG_MILSTDCFG_RSTDIS_Msk | RCU_MILSTDCFG_MILSTDCFG_CLKEN_Msk;
  RCU->HRSTCFG_bit.MILSTD1EN = 1;
  RCU->HCLKCFG_bit.MILSTD1EN = 1;
  // прерывания
  // MILSTD1->IENR = MILSTD_IENR_BCEVE_Msk | MILSTD_IENR_BCDE_Msk;
  /*Descriptors fill*/
  mko_ptr->MKOBC_DescListRx_ptr = &MKOBC_DescListRx;
  mko_ptr->MKOBC_DescListTx_ptr = &MKOBC_DescListTx;
  /* Rx */
  mko_ptr->MKOBC_DescListRx_ptr->Word0.u32 = 0;
  mko_ptr->MKOBC_DescListRx_ptr->Word0.uf.retmd = 0;
  mko_ptr->MKOBC_DescListRx_ptr->Word0.uf.nret = 0;
  mko_ptr->MKOBC_DescListRx_ptr->Word0.uf.stbus = 1;
  mko_ptr->MKOBC_DescListRx_ptr->Word0.uf.gap = 1;
	mko_ptr->MKOBC_DescListRx_ptr->Word0.uf.stime = 0x1F;
  mko_ptr->MKOBC_DescListRx_ptr->Word1.u32 = 0;
  mko_ptr->MKOBC_DescListRx_ptr->Word1.uf.rtto = 0xF;
  mko_ptr->MKOBC_DescListRx_ptr->Word1.uf.tr = 1;
  mko_ptr->MKOBC_DescListRx_ptr->BuffAddr = (uint32_t)mko_ptr->MKOBC_BuffRx;
  mko_ptr->MKOBC_DescListRx_ptr->BranchCond.u32 = 0x800000FF;
  /* Tx */
  mko_ptr->MKOBC_DescListTx_ptr->Word0.u32 = 0;
  mko_ptr->MKOBC_DescListTx_ptr->Word0.uf.retmd = 0;
  mko_ptr->MKOBC_DescListTx_ptr->Word0.uf.nret = 0;
  mko_ptr->MKOBC_DescListTx_ptr->Word0.uf.stbus = 1;
  mko_ptr->MKOBC_DescListTx_ptr->Word0.uf.gap = 1;
	mko_ptr->MKOBC_DescListTx_ptr->Word0.uf.stime = 0x1F;
  mko_ptr->MKOBC_DescListTx_ptr->Word1.u32 = 0;
  mko_ptr->MKOBC_DescListTx_ptr->Word1.uf.tr = 0;
  mko_ptr->MKOBC_DescListTx_ptr->Word1.uf.rtto = 0xF;
  mko_ptr->MKOBC_DescListTx_ptr->BuffAddr = (uint32_t)mko_ptr->MKOBC_BuffTx;
  mko_ptr->MKOBC_DescListTx_ptr->BranchCond.u32 = 0x800000FF;
  //
  mko_ptr->error = 0x00;
  mko_ptr->error_cnt = 0x00;
  //
  mko_ptr->inst = (MILSTD_TypeDef_Simple*)MILSTD1;
}

/**
 * @brief 
 * 
 * @param mko_ptr 
 * @param bus 
 */
void mko_bc_set_bus(typeMKOBCStruct *mko_ptr, uint8_t bus)
{
  mko_ptr->used_bus = ((bus & 0x01) == 0) ? MKO_BC_BUS_A : MKO_BC_BUS_B;
}

/**
 * @brief голая отправка данных (от АА)
 * 
 * @param mko_ptr 
 * @param rt_addr 
 * @param subaddr 
 * @param leng 
 * @param buff 
 * @return uint32_t 
 */
uint32_t mko_bc_rx(typeMKOBCStruct *mko_ptr, uint8_t rt_addr, uint8_t subaddr, uint8_t leng, uint16_t *buff) 
{
  int i, cnt;
  mko_ptr->MKOBC_DescListRx_ptr->Word1.uf.rtad1 = rt_addr & 0x1F;
  mko_ptr->MKOBC_DescListRx_ptr->Word1.uf.rtsa1 = subaddr & 0x1F;
  cnt = (leng >= 32) ? 32 : (leng & 0x1F);
  mko_ptr->MKOBC_DescListRx_ptr->Word1.uf.wcmc = leng & 0x1F;
  /*act*/
  mko_ptr->MKOBC_DescListRx_ptr->Result.uf.flg0 = 1;
  MILSTD1->BCLNP = (uint32_t)mko_ptr->MKOBC_DescListRx_ptr;
  MILSTD1->BCACT = (0x1552<<16) | MILSTD_BCACT_SCSRT_Msk;
  while(mko_ptr->MKOBC_DescListRx_ptr->Result.uf.flg0);
  if(mko_ptr->MKOBC_DescListRx_ptr->Result.uf.tfrst == 0) {
    for(i=0; i<cnt; i++)  buff[i] = mko_ptr->MKOBC_BuffRx[i];
    }
  return mko_ptr->MKOBC_DescListRx_ptr->Result.u32;
}

/**
 * @brief голый прием данных (от АА)
 * 
 * @param mko_ptr 
 * @param rt_addr 
 * @param subaddr 
 * @param leng 
 * @param buff 
 * @return uint32_t 
 */
uint32_t mko_bc_tx(typeMKOBCStruct *mko_ptr,uint8_t rt_addr, uint8_t subaddr, uint8_t leng, uint16_t *buff)
{
  int i, cnt;
  mko_ptr->MKOBC_DescListTx_ptr->Word1.uf.rtad1 = rt_addr & 0x1F;
  subaddr = subaddr & 0x1F;
  mko_ptr->MKOBC_DescListTx_ptr->Word1.uf.rtsa1 = subaddr;
  if((subaddr == 0)||(subaddr == 0x1F))
    cnt = 0;
  else
    cnt = (leng >= 32) ? 32 : (leng & 0x1F);
  mko_ptr->MKOBC_DescListTx_ptr->Word1.uf.wcmc = leng & 0x1F;
  for(i=0; i<cnt; i++)  mko_ptr->MKOBC_BuffTx[i] = buff[i];
  /*act*/
  mko_ptr->MKOBC_DescListTx_ptr->Result.uf.flg0 = 1;
  MILSTD1->BCLNP = (uint32_t)mko_ptr->MKOBC_DescListTx_ptr;
  MILSTD1->BCACT = (0x1552<<16) | MILSTD_BCACT_SCSRT_Msk;
  while(mko_ptr->MKOBC_DescListTx_ptr->Result.uf.flg0);
  return mko_ptr->MKOBC_DescListTx_ptr->Result.u32;
}

/**
  * @brief  чтение данных с подадреса по номеру для КШ c перебором шин
  * @param  mko_ptr указатель на программную модель устройства
  * @param  addr адрес устройства
  * @param  subaddr номер субадреса
  * @param  data данные для записи (длина - 32 слова)
  * @param  leng количество данных
	* @retval 1 - норма, 0 - ошибка
  */
uint8_t mko_bc_transaction_start_with_bus_find(typeMKOBCStruct *mko_ptr, uint8_t mode, uint8_t addr, uint8_t subaddr, uint16_t* data, uint8_t leng)
{
	volatile uint16_t cw = 0;
  cw = (addr << 11) | ((mode & 0x01) << 10) | ((subaddr&0x1F) << 5) | ((leng & 0x1F) << 0);
  if (mko_bc_transaction_start(mko_ptr, mode, addr, subaddr, data, leng)) {
    return 1;
  }
  else{
    mko_bc_change_bus(mko_ptr);
    if(mko_bc_transaction_start(mko_ptr, mode, addr, subaddr, data, leng)){
			return 1;
		}
  }
	return 0;
}

/**
  * @brief  чтение данных с подадреса по номеру для КШ
  * @param  mko_ptr указатель на программную модель устройства
  * @param  addr адрес устройства
  * @param  subaddr номер субадреса
  * @param  data данные для записи (длина - 32 слова)
  * @param  leng количество данных
  * @retval uint8_t 0 - error, 1 - ok
  */
uint8_t mko_bc_transaction_start(typeMKOBCStruct *mko_ptr, uint8_t mode, uint8_t addr, uint8_t subaddr, uint16_t* data, uint8_t leng)
{
	volatile uint16_t cw = 0;
  //заполняем командное слово 1
	cw = (addr << 11) | ((mode & 0x01) << 10) | ((subaddr&0x1F) << 5) | ((leng & 0x1F) << 0);
  return mko_bc_transaction_start_by_cw(mko_ptr, cw, data);
}

/**
  * @brief  чтение данных с подадреса по номеру для КШ
  * @param  mko_ptr указатель на программную модель устройства
  * @param  cw командное слово
  * @retval uint8_t 0 - error, 1 - ok
  */
uint8_t mko_bc_transaction_start_by_cw(typeMKOBCStruct *mko_ptr, uint16_t cw_var, uint16_t* data)
{
  uint8_t leng;
	volatile typeCommandWord cw;
  typeMKOBC_DescResult result;
  //
	mko_ptr->cw.whole = cw_var;
  cw.whole = cw_var;
  leng = ((cw.field.leng  & 0x1F) == 0) ? 32 : (cw.field.leng & 0x1F);
  while(Timers_Status(1)) {};
  //
  switch(cw.field.rd_wr){
    case MKO_BC_MODE_READ:
      Timers_Start(1, 1);
      result.u32 = mko_bc_rx(mko_ptr, cw.field.addr, cw.field.sub_addr, leng, data);
      switch (result.uf.tfrst){
        case TFRST_SUCCESS:
          return 1;
        default:
          mko_ptr->error_cnt ++;
          mko_ptr->error = result.uf.tfrst;
          return 0;
      }
    case MKO_BC_MODE_WRITE:
      Timers_Start(1, 1);
      result.u32 = mko_bc_tx(mko_ptr, cw.field.addr, cw.field.sub_addr, cw.field.leng, data);
      switch (result.uf.tfrst){
        case TFRST_SUCCESS:
          return 1;
        case TFRST_ERROR_BITS:
          //TODO: возможно стоит дополнительно разобрать бит занятости
          memset((uint8_t*)data, 0xFE, leng*2);
          mko_ptr->error_cnt ++;
          mko_ptr->error = result.uf.tfrst;
          return 0;
        default:
          memset((uint8_t*)data, 0xFE, leng*2);
          mko_ptr->error_cnt ++;
          mko_ptr->error = result.uf.tfrst;
          return 0;
      }
		default:
			mko_ptr->error_cnt ++;
			mko_ptr->error = (1<<7);
			return 0;
  }
}

/**
 * @brief 
 * 
 * @param mko_ptr 
 */
uint8_t mko_bc_change_bus(typeMKOBCStruct *mko_ptr)
{
  // меняем значение на противоположное
  mko_ptr->used_bus = (mko_ptr->used_bus == MKO_BC_BUS_A) ? MKO_BC_BUS_B : MKO_BC_BUS_A;
  return mko_ptr->used_bus;
}

/**
  * @brief  возврат значения ошибок модуля МКО
  * @param  mko_ptr указатель на программную модель устройства
	* @param  error указатель на переменную с ошибкой МКО
	* @param  error_cnt указатель на переменную с счетчиком ошибок МКО
  */
void mko_bc_get_error(typeMKOBCStruct *mko_ptr, uint8_t* error, uint8_t* error_cnt)
{
	*error = mko_ptr->error;
	*error_cnt = mko_ptr->error_cnt;
}

void MILSTD1_IRQHandler(void)
{
  if (MILSTD1->IR & MILSTD_IR_BCEV_Msk){ // Успешная передача КШ
    //
  }
  else if(MILSTD1->IR & MILSTD_IR_BCD_Msk){ // Ошибка передачи
    //
  }
  // сброс прерывания
  MILSTD1->IR = MILSTD1->IR;
}
