#ifndef _CYCLOGRAMMA_H_
#define _CYCLOGRAMMA_H_

#include <string.h>
#include "main.h"

// дефайны для переменных
#define CYCLO_MAX_STEP 16

#define CYCLO_MODE_OFF 0
#define CYCLO_MODE_WORK 1
#define CYCLO_MODE_READY 2
#define CYCLO_MODE_PAUSE 3

#define CYCLO_DO_NOTHING (cyclo_do_nothing)

// структуры данных
#pragma pack(push, 2)

/** 
  * @brief  структура с отдельным шагом циклограммы
  * @param func функция обработчик шага циклограммы
*/
typedef  struct
{
  int32_t (*func) (void* ctrl_struct, uint8_t* data);
  void* ctrl_struct;  // возможно получится через отдельные шаги сделать совместные циклограммы через различные управляющие структуры
  uint32_t repeat_num;
  uint32_t delay_to_next_step_ms;
  uint8_t data[32];
}typeCyclogramaStep;

/** 
  * @brief  структура управления отдельной циклограммой
  */
typedef  struct
{
  char name[16];
  typeCyclogramaStep step[CYCLO_MAX_STEP];
  uint32_t step_number;
  uint8_t mode;
  uint8_t current_step;
  uint32_t repeat_cnt;
  int32_t func_ret_val;
  uint32_t last_step_time, last_step_duration, pause_time;
  //
  uint32_t last_call_time_ms;
}typeCyclograma;

#pragma pack(pop)
//
void cyclo_init(typeCyclograma* cyclo_ptr, char* name);
int8_t cyclo_add_step(typeCyclograma* cyclo_ptr, int32_t (*func) (void*, uint8_t*), void* ctrl_struct, uint32_t repeats, uint32_t delay_ms, uint8_t *data);
uint8_t cyclo_handler(typeCyclograma* cyclo_ptr, uint32_t time_ms);
int8_t cyclo_start(typeCyclograma* cyclo_ptr);
void cyclo_stop(typeCyclograma* cyclo_ptr);
void cyclo_pause(typeCyclograma* cyclo_ptr);
uint8_t cyclo_get_operation_status(typeCyclograma* cyclo_ptr);
int32_t cyclo_do_nothing(void* ctrl_struct, uint8_t *data);
//
#endif
