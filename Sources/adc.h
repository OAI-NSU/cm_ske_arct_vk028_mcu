#ifndef _ADC_H_
#define _ADC_H_

#include "CM4/K1921VK028.h"
#include "system_K1921VK028.h"
#include "task_planner.h"

#define ADC_VAL_0V0 ( 0. )
#define ADC_VAL_3V3 ( 4095.)
#define ADC_A ((3.3-0.0) / (ADC_VAL_3V3 - ADC_VAL_0V0))
#define ADC_B (3.3-(ADC_A*ADC_VAL_3V3))

#define ADC_PROCESS_PERIOD_MS   500

#define ADC_CHANNEL_NUM         24

/**
  * @brief  структура управления АЦП
  */
typedef struct
{
    volatile uint16_t data[ADC_CHANNEL_NUM];
    uint8_t error_cnt;
    float voltage_arr[ADC_CHANNEL_NUM];
    //
    uint64_t last_call_time_us;
}typeADCStruct;

void adc_init(typeADCStruct* adc_ptr);
void __adc_seq_start(void);
int8_t adc_process_tp(void* ctrl_struct, uint64_t time_us, typeProcessInterfaceStruct* interface);
//
float adc_ch_voltage(typeADCStruct* adc_ptr, uint8_t ch_num);
void adc_voltage_update(typeADCStruct* adc_ptr);
//
void  __adc_calibration_init(float* arr, float val, uint16_t num);
//

__weak void ADC_SEQ0_CallBack(void);

#endif
