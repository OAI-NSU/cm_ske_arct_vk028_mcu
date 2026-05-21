#ifndef _STM_H_
#define _STM_H_

#include "main.h"
#include "stm_settings.h"
#include "gpio.h"
#include "internal_bus.h"

#define STM_HANDLER_INTERVAL_MS (100)

#pragma pack(push, 2)

/** 
  * @brief  структура управления STM-каналом
  */
typedef struct
{
  uint8_t type;
  uint8_t const_state;         //! состояние СТМ сигнала по умолчанию, при отсутствии временных состояний
  uint8_t temporary_state;     //! значение временного состояния СТМ сигнала
  uint32_t temporary_timeout_ms;  //! время до перехода в const-значение
  type_SINGLE_GPIO *io_ptr;
  uint8_t current_state;
} type_STM_Channel_Model;

/** 
  * @brief  структура управления STM
  */
typedef struct
{
  type_GPIO_OAI_cm *cm_io_ptr;
  //
  type_STM_Channel_Model ch[STM_NUM];
  uint16_t state, state_old;
  //
  uint64_t call_interval_us;
  uint64_t last_call_time_us;
} type_STM_Model;

#pragma pack(pop)

//Single channel
int8_t stm_ch_init(type_STM_Channel_Model *stm_ch_ptr, type_SINGLE_GPIO *io_ptr, uint8_t value);
void stm_ch_process(type_STM_Channel_Model *stm_ch_ptr, uint32_t period_ms);
void stm_ch_const_set(type_STM_Channel_Model *stm_ch_ptr, uint8_t val);
void stm_ch_temporary_set(type_STM_Channel_Model *stm_ch_ptr, uint8_t val, uint32_t timeout_ms);
uint8_t stm_ch_get_state(type_STM_Channel_Model *stm_ch_ptr);
//General model
void stm_init(type_STM_Model *stm_ptr, type_GPIO_OAI_cm* cm_io_ptr);
int8_t stm_process_tp(void* ctrl_struct, uint64_t time_us, typeProcessInterfaceStruct* interface);
void stm_process(type_STM_Model *stm_ptr, uint32_t period_ms);
uint32_t __stm_update_state(type_STM_Model *stm_ptr);
void __stm_update_outputs(type_STM_Model *stm_ptr);
void __stm_update_value(type_STM_Model *stm_ptr);
void stm_single_ch_const_set(type_STM_Model *stm_ptr, uint8_t num, uint32_t val);
void stm_single_ch_temporary_set(type_STM_Model *stm_ptr, uint8_t num, uint32_t val, uint32_t timeout_ms);
void stm_const_set(type_STM_Model *stm_ptr, uint32_t val);
void stm_temporary_set(type_STM_Model *stm_ptr, uint32_t val, uint32_t timeout_ms);
uint8_t stm_single_ch_get_state(type_STM_Model *stm_ptr, uint8_t num);
uint32_t stm_get_state(type_STM_Model *stm_ptr);
#endif
