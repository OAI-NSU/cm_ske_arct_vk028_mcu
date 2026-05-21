/**
 * @file wdt.c
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2024-02-21
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#include "wdt.h"


/**
 * @brief Инициализация сторожевого таймера
 * 
 * @return int 
 */
void WDT_Init(void) 
{
  RCU->WDTCFG_bit.DIVN = 24;   // 100 / 50 = 2 MHz
  RCU->WDTCFG_bit.CLKSEL = 3;  //from PLL
  RCU->WDTCFG|= RCU_MILSTDCFG_MILSTDCFG_DIVEN_Msk | RCU_MILSTDCFG_MILSTDCFG_RSTDIS_Msk | RCU_MILSTDCFG_MILSTDCFG_CLKEN_Msk;
  //
  WDT->LOAD = WD_TIME;
  //
  WDT->CTRL_bit.INTEN = 1;
  WDT->CTRL_bit.RESEN = 1;
}

/**
 * @brief Функция перезапуска сторожевого таймера
 * 
 * @return int 
 */
void WDT_Reset(void)
{
  WDT->INTCLR = WDT_INTCLR_WDTCLR_Msk;
}

void WDT_IRQHandler(void)
{
  printf("\n!!! WARNING !!! WDT timer is finish first time\n");
  __DSB();
  while(1){
  }
}
