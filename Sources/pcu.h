#ifndef _PCU_H_
#define _PCU_H_

#include "main.h"
#include "gpio_conf.h"
#include "task_planner.h"

#pragma pack(push, 2)

#define PСU_CMD_PER_LVL_NUMS      (6)

#define PCU_CMD_THRESHOLD_MS      (1000)
#define PCU_CMD_WAITING_MS        (10000)

#define PCU_HANDLER_INTERVAL_MS   (50)

/**
  * @brief  состояние автоматов
*/
enum pcu_state_enum
{
	PCU_STATE_IDLE, 
  PCU_STATE_CMD_THRESHOLD, 
  PCU_STATE_CMD_WAITING, 
  PCU_STATE_CMD_ACTION_REG,
  PCU_STATE_CMD_RESET, 
  PCU_STATE_CMD_NUM
};

/**
  * @brief  события переходов
*/
enum pcu_event_enum
{
	PCU_EVENT_NO_ONE,
	PCU_EVENT_IRQ,
  PCU_EVENT_1S_EXPIRED, 
  PCU_EVENT_10S_EXPIRED,
  PCU_EVENT_NUM
};

/**
  * @brief  статусы дополнительного таймера
*/
enum pcu_timer_status
{
	PCU_TIMER_STATUS_IDLE,
	PCU_TIMER_STATUS_WORK,
  PCU_TIMER_STATUS_FINISH, 
  PCU_TIMER_STATUS_NUM
};

/**
  * @brief  статусы дополнительного таймера
*/
enum pcu_status
{
	PCU_STATUS_ERROR_EVENT,
	PCU_STATUS_ERROR_DOUBLE_EVENT,
  PCU_STATUS_ERROR_LEVEL,
  PCU_STATUS_ERROR_STATE,
  PCU_STATUS_ERROR_UNKNOWN_CMD,
  PCU_STATUS_ERROR_NO_CMD,
	PCU_STATUS_NUM
};


typedef struct
{
  uint64_t timer_us;
  uint64_t goal_us;
  uint8_t status;
} type_PCU_Timer;

/** 
  * @brief  структура контроля прохождения команд по ПКУ
  */
typedef struct
{
  uint16_t status;
  uint16_t err_cnter;
  uint16_t cmd_cnter;
  uint16_t cmd_cnter_by_num[2][PСU_CMD_PER_LVL_NUMS];

  uint8_t state; 
  uint32_t event;

  type_PCU_Timer timer[2];

  uint8_t last_cmd_num[2];
  uint8_t cmd_ready;
  uint8_t current_lvl;
  uint8_t cmd_num_on_level[2];

  uint64_t last_call_time_us, call_interval_us;

} type_GPIO_PCU;

#pragma pack(pop)

//
void pcu_init(type_GPIO_PCU* pcu_ptr);
void pcu_reset(type_GPIO_PCU* pcu_ptr);

int8_t pcu_process_tp(void* ctrl_struct, uint64_t time_us, typeProcessInterfaceStruct* interface);

int8_t pcu_state_machine_process(type_GPIO_PCU* pcu_ptr);
void pcu_cmd_reset(type_GPIO_PCU* pcu_ptr);
void pcu_set_event(type_GPIO_PCU* pcu_ptr, uint8_t event);
uint8_t pcu_extract_event(type_GPIO_PCU* pcu_ptr);

int8_t pcu_cmd_registration(type_GPIO_PCU* pcu_ptr);
int16_t pcu_cmd_extraction(type_GPIO_PCU* pcu_ptr);

void pcu_timer_init(type_PCU_Timer* pcu_timer_ptr);
void pcu_timer_start(type_PCU_Timer* pcu_timer_ptr, uint32_t time_ms);
uint8_t pcu_timer_process(type_PCU_Timer* pcu_timer_ptr, uint64_t interval_us);
uint8_t pcu_timer_get_status(type_PCU_Timer* pcu_timer_ptr);


#endif
