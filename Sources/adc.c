/**
  ******************************************************************************
  * @file           : adc.c
  * @version        : v1.0
  * @brief          : библиотека для работы с АЦП0
  * @note           : библиотека построена псевдопотоке на прерывании //todo: поменять на работу с DMA
  * @author					: Стюф Алексей/Alexe Styuf <a-styuf@yandex.ru>
	* @date						: 2024.02.08
  ******************************************************************************
  */

#include "adc.h"

/**
  * @brief  инициализация структуры ADC
	* @param  adc_ptr указатель на структуру управления
  */
void adc_init(typeADCStruct* adc_ptr)
{
  //
  float cal_a[ADC_CHANNEL_NUM];
  float cal_b[ADC_CHANNEL_NUM];
  //
  __adc_calibration_init(cal_a, ADC_A, ADC_CHANNEL_NUM);
  __adc_calibration_init(cal_b, ADC_B, ADC_CHANNEL_NUM);
  adc_ptr->error_cnt = 0;
  //
  PMU->ADCPC = PMU_ADCPC_LDOEN0_Msk | PMU_ADCPC_LDOEN1_Msk | PMU_ADCPC_LDOEN2_Msk | PMU_ADCPC_LDOEN3_Msk ;  // LDO for ADC0-3
  while((PMU->ADCPC & PMU_ADCPC_LDORDY0_Msk) == 0);
  while((PMU->ADCPC & PMU_ADCPC_LDORDY1_Msk) == 0);
  while((PMU->ADCPC & PMU_ADCPC_LDORDY2_Msk) == 0);
  while((PMU->ADCPC & PMU_ADCPC_LDORDY3_Msk) == 0);
  /* Init: clk - PLLCLK,  Fadc = 100 MHz / 10 = 10 MHz */
  RCU->ADCCFG |= (1 << 8) | (5 << 24) | RCU_ADCCFG_DIVEN_Msk | RCU_ADCCFG_RSTDIS_Msk | RCU_ADCCFG_CLKEN_Msk;
  /* Setup and Calibr.*/
  ADC->ACTL[0].ACTL = (3<<4) | ADC_ACTL_ACTL_CALEN_Msk | ADC_ACTL_ACTL_ADCEN_Msk;  // 12bit, Calibr., Start
  ADC->ACTL[1].ACTL = (3<<4) | ADC_ACTL_ACTL_CALEN_Msk | ADC_ACTL_ACTL_ADCEN_Msk;  // 12bit, Calibr., Start
  ADC->ACTL[2].ACTL = (3<<4) | ADC_ACTL_ACTL_CALEN_Msk | ADC_ACTL_ACTL_ADCEN_Msk;  // 12bit, Calibr., Start
  ADC->ACTL[3].ACTL = (3<<4) | ADC_ACTL_ACTL_CALEN_Msk | ADC_ACTL_ACTL_ADCEN_Msk;  // 12bit, Calibr., Start 
  /*sequncer*/
  ADC->EMUX = 0;  // запуск через запись в регистр -> EM0
  ADC->SEQ[0].SCCTL = (23<<16);  //max очередь в FIFO = 24 (24 каналов), 0 перезапусков
  ADC->SEQ[0].SRQCTL = (3<<9) | ADC_SEQ_SRQCTL_QAVGEN_Msk | 23;  //усреднение по 8-ми, RQ=0..23
  ADC->SEQ[0].SRQSEL0 = (3<<24) | (2<<16) | (1<<8) | (0<<0);  // ch = 0,1,2,3
  ADC->SEQ[0].SRQSEL1 = (7<<24) | (6<<16) | (5<<8) | (4<<0);  // ch = 4,5,6,7
  ADC->SEQ[0].SRQSEL2 = (11<<24) | (10<<16) | (9<<8) | (8<<0);  // ch = 8,9,10,11
  ADC->SEQ[0].SRQSEL3 = (15<<24) | (14<<16) | (13<<8) | (12<<0);  // ch = 12,13,14,15
  ADC->SEQ[0].SRQSEL4 = (19<<24) | (18<<16) | (17<<8) | (16<<0);  // ch = 16,17,18,19
  ADC->SEQ[0].SRQSEL5 = (23<<24) | (22<<16) | (21<<8) | (20<<0);  // ch = 20,21,22,23
  ADC->SEQEN = 1;
  /*interrupt*/
  ADC->IM = ADC_IM_SEQIM0_Msk;
  NVIC_EnableIRQ(ADC_SEQ0_IRQn);
  /*Start*/
  while((ADC->ACTL[0].ACTL & ADC_ACTL_ACTL_ADCRDY_Msk) == 0);
  while((ADC->ACTL[1].ACTL & ADC_ACTL_ACTL_ADCRDY_Msk) == 0);
  while((ADC->ACTL[2].ACTL & ADC_ACTL_ACTL_ADCRDY_Msk) == 0);
  while((ADC->ACTL[3].ACTL & ADC_ACTL_ACTL_ADCRDY_Msk) == 0);
  //
}

void __adc_seq_start(void)
{
  ADC->SEQSYNC |= ADC_SEQSYNC_GSYNC_Msk | ADC_SEQSYNC_SYNC0_Msk;
}

/**
  * @brief  функция для запуска в планировщике задач
  * @note  не обязательна к запуску, создана для удобства контроля напряжения на каналах
	* @param  ctrl_struct указатель на програмную модель устройства
	* @param  time_us глобальное время
  */
int8_t adc_process_tp(void* ctrl_struct, uint64_t time_us, typeProcessInterfaceStruct* interface)
{
  typeADCStruct* adc_ptr = (typeADCStruct*)ctrl_struct;
  //
  if ((time_us - adc_ptr->last_call_time_us) > (ADC_PROCESS_PERIOD_MS*1000)) {
    adc_ptr->last_call_time_us = time_us;
    adc_voltage_update(adc_ptr);
    __adc_seq_start();
    return 1;
  }
  else{
    return 0;
  }
}

/**
  * @brief  запрос данных канала АЦП в В
	* @param  adc_ptr указатель на програмную модель устройства
	* @param  ch_num номер канала
	* @retval значение канала АЦП в В
  */
float adc_ch_voltage(typeADCStruct* adc_ptr, uint8_t ch_num)
{
  if(ch_num<ADC_CHANNEL_NUM){
    return adc_ptr->voltage_arr[ch_num];
  }
  else{
    adc_ptr->error_cnt += 1;
    return 0;
  }
}

void adc_voltage_update(typeADCStruct* adc_ptr)
{
  for (uint8_t i=0; i<ADC_CHANNEL_NUM; i++)
  {
    adc_ptr->voltage_arr[i] = ADC_A*adc_ptr->data[i] + ADC_B;
  }
}


/**
  * @brief  _static_ заполнение массива одинаковыми значениями по параметру длины
  * @param arr указатель на массив для заполнения
  * @param val значение для заполнения
  * @param num количество элементов для заполнения
  */
void  __adc_calibration_init(float* arr, float val, uint16_t num)
{
  uint16_t i=0;
  for(i=0; i<num; i++){
    arr[i] = val;
  }
}

void ADC_SEQ0_IRQHandler() {
  ADC_SEQ0_CallBack();
}

__weak void ADC_SEQ0_CallBack(void)
{
  //
}
