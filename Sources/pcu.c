  /**
  ******************************************************************************
  * @file           : pcu.c
  * @version        : v1.0
  * @brief          : функция - обработчик команд ПКУ
  * @author			    : Стюф Алексей/Alexe Styuf <a-styuf@yandex.ru>
  ******************************************************************************
  */

#include "pcu.h"

/**
 * @brief инициализация программного модуля отслеживания ПКУ
 * 
 * @param pcu_ptr 
 */
void pcu_init(type_GPIO_PCU* pcu_ptr)
{
  pcu_reset(pcu_ptr);
}

/**
 * @brief Приведение программного модуля отслеживания ПКУ в исходное состояние
 * 
 * @param pcu_ptr 
 */
void pcu_reset(type_GPIO_PCU* pcu_ptr)
{
  memset((uint8_t*)pcu_ptr, 0x00, sizeof(type_GPIO_PCU)); 
  //
  pcu_timer_init(&pcu_ptr->timer[0]);
  pcu_timer_init(&pcu_ptr->timer[1]);
}

/**
 * @brief Стандартный обработчик планировщика задач
 * 
 * @param ctrl_struct 
 * @param time_us 
 * @param interface 
 * @return int8_t 
 */
int8_t pcu_process_tp(void* ctrl_struct, uint64_t time_us, typeProcessInterfaceStruct* interface)
{
	uint8_t retval=0;
  uint8_t timer_status;
	type_GPIO_PCU* pcu_ptr = (type_GPIO_PCU*)ctrl_struct;
	//
	if ((time_us - pcu_ptr->last_call_time_us) > (PCU_HANDLER_INTERVAL_MS*1000)) {
		pcu_ptr->call_interval_us = time_us - pcu_ptr->last_call_time_us;
		pcu_ptr->last_call_time_us = time_us;
		// user code begin
    timer_status = pcu_timer_process(&pcu_ptr->timer[0], pcu_ptr->call_interval_us);
    if (timer_status == PCU_TIMER_STATUS_FINISH){
      pcu_timer_init(&pcu_ptr->timer[0]);
      pcu_set_event(pcu_ptr, PCU_EVENT_1S_EXPIRED);
    }
    timer_status = pcu_timer_process(&pcu_ptr->timer[1], pcu_ptr->call_interval_us);
    if (timer_status == PCU_TIMER_STATUS_FINISH){
      pcu_timer_init(&pcu_ptr->timer[1]);
      pcu_set_event(pcu_ptr, PCU_EVENT_10S_EXPIRED);
    }
    
    pcu_state_machine_process(pcu_ptr);
		// user code end
		retval = 1;
	}
  return retval;
}

int8_t pcu_state_machine_process(type_GPIO_PCU* pcu_ptr)
{
  uint8_t event = PCU_EVENT_NO_ONE;
  //
  event = pcu_extract_event(pcu_ptr);
  //
  switch(pcu_ptr->state){
    case PCU_STATE_IDLE:
      if (event == PCU_EVENT_IRQ){
        //  функция перехода в следующее состояние
        pcu_timer_start(&pcu_ptr->timer[0], PCU_CMD_THRESHOLD_MS);
        //
        pcu_ptr->state = PCU_STATE_CMD_THRESHOLD;
      }
      break;
    case PCU_STATE_CMD_THRESHOLD:
      if (event == PCU_EVENT_1S_EXPIRED){
        //  функция перехода в следующее состояние
        pcu_timer_start(&pcu_ptr->timer[1], PCU_CMD_WAITING_MS);
        if (pcu_ptr->current_lvl < 2){
          pcu_ptr->cmd_num_on_level[pcu_ptr->current_lvl]++;
        }
        else{
          pcu_ptr->err_cnter ++;
          pcu_ptr->status |= (1 << PCU_STATUS_ERROR_LEVEL);
        }
        //
        pcu_ptr->state = PCU_STATE_CMD_WAITING;
      }
      break;
    case PCU_STATE_CMD_WAITING:
      if (event == PCU_EVENT_IRQ){
        //  функция перехода в следующее состояние
        pcu_timer_start(&pcu_ptr->timer[0], PCU_CMD_THRESHOLD_MS);
        //
        pcu_ptr->state = PCU_STATE_CMD_THRESHOLD;
      }
      else if (event == PCU_EVENT_10S_EXPIRED) {
        if (pcu_ptr->current_lvl == 0) {
          // функция перехода в следующее состояние
          pcu_ptr->current_lvl++;
          pcu_timer_start(&pcu_ptr->timer[1], PCU_CMD_WAITING_MS);
          //
          pcu_ptr->state = PCU_STATE_CMD_WAITING;
          //
        }
        else if ((pcu_ptr->current_lvl == 1) && (pcu_ptr->cmd_num_on_level[1] == 0)) {
          // функция перехода в следующее состояние
          pcu_ptr->err_cnter ++;
          pcu_ptr->status |= (1 << PCU_STATUS_ERROR_NO_CMD);
          //
          pcu_ptr->state = PCU_STATE_CMD_RESET;
        }
        else if ((pcu_ptr->current_lvl == 1) && (pcu_ptr->cmd_num_on_level[1] != 0)) {
          // функция перехода в следующее состояние
          //
          pcu_ptr->state = PCU_STATE_CMD_ACTION_REG;
        }
      }
      break;
    case PCU_STATE_CMD_ACTION_REG:
      // функция перехода в следующее состояние
      pcu_cmd_registration(pcu_ptr);
      //
      pcu_ptr->state = PCU_STATE_CMD_RESET;
      break;
    case PCU_STATE_CMD_RESET:
      // функция перехода в следующее состояние
      pcu_cmd_reset(pcu_ptr);
      //
      pcu_ptr->state = PCU_STATE_IDLE;
      break;
    default:
      // функция перехода в следующее состояние
      pcu_cmd_reset(pcu_ptr);
      pcu_ptr->err_cnter ++;
      pcu_ptr->status |= (1 << PCU_STATUS_ERROR_STATE);
      //
      pcu_ptr->state = PCU_STATE_IDLE;
      break;
  }
	return 0;
}

/**
 * @brief функция сброса набора данных отдельной команды
 * 
 * @param pcu_ptr 
 */
void pcu_cmd_reset(type_GPIO_PCU* pcu_ptr)
{
  pcu_timer_init(&pcu_ptr->timer[0]);
  pcu_timer_init(&pcu_ptr->timer[1]);
  //
  pcu_ptr->last_cmd_num[0] = 0;
  pcu_ptr->last_cmd_num[1] = 0;
  pcu_ptr->cmd_ready = 0;
  pcu_ptr->current_lvl = 0;
  for (uint8_t lvl_num = 0; lvl_num < 2; lvl_num++){
    pcu_ptr->cmd_num_on_level[lvl_num] = 0;
  }
}

void pcu_set_event(type_GPIO_PCU* pcu_ptr, uint8_t event)
{
  if (event < PCU_EVENT_NUM){
    pcu_ptr->event |= 1 << event;
  }
  else{
    pcu_ptr->err_cnter ++;
    pcu_ptr->status |= (1 << PCU_STATUS_ERROR_EVENT);
  }
  return;
}

uint8_t pcu_extract_event(type_GPIO_PCU* pcu_ptr)
{
  for (uint8_t event_num = 0; event_num < PCU_EVENT_NUM; event_num++){
    if ((pcu_ptr->event >> event_num) & 0x01){
      pcu_ptr->event &= ~(1<<event_num);
      return event_num;
    }
  } 
  return 0;
}

/**
 * @brief функция для вызова в прерывании от GPIO ПКУ 
 * 
 * @param pcu_ptr 
 * @param cmd_num номер команды для отработки
 * @return int8_t 
 */
int8_t pcu_cmd_registration(type_GPIO_PCU* pcu_ptr)
{
  if ((pcu_ptr->cmd_num_on_level[0] <= PСU_CMD_PER_LVL_NUMS) && (pcu_ptr->cmd_num_on_level[0] <= PСU_CMD_PER_LVL_NUMS)){
    pcu_ptr->last_cmd_num[0] = pcu_ptr->cmd_num_on_level[0];
    pcu_ptr->last_cmd_num[1] = pcu_ptr->cmd_num_on_level[1];
    pcu_ptr->cmd_ready = 1;
    return 1;
  }
  else{
    pcu_ptr->err_cnter ++;
    pcu_ptr->status |= (1 << PCU_STATUS_ERROR_UNKNOWN_CMD);
  }
  return 0;
}

/**
 * @brief функция вызова команды по его номеру
 * 
 * @param pcu_ptr 
 * @return int16_t 
 */
int16_t pcu_cmd_extraction(type_GPIO_PCU* pcu_ptr)
{
  if(pcu_ptr->cmd_ready){
    pcu_ptr->cmd_ready = 0;
    return ((pcu_ptr->last_cmd_num[0] & 0xFF) << 8) | (pcu_ptr->last_cmd_num[1] & 0xFF);
  }
  return 0;
}


void pcu_timer_init(type_PCU_Timer* pcu_timer_ptr)
{
  pcu_timer_ptr->goal_us = 0;
  pcu_timer_ptr->timer_us = 0;
  pcu_timer_ptr->status = PCU_TIMER_STATUS_IDLE;
}

void pcu_timer_start(type_PCU_Timer* pcu_timer_ptr, uint32_t time_ms)
{
  pcu_timer_ptr->status = PCU_TIMER_STATUS_WORK;
  pcu_timer_ptr->goal_us = time_ms*1000;
  pcu_timer_ptr->timer_us = 0;
}

uint8_t pcu_timer_process(type_PCU_Timer* pcu_timer_ptr, uint64_t interval_us)
{
  if (pcu_timer_ptr->status == PCU_TIMER_STATUS_WORK){
    pcu_timer_ptr->timer_us += interval_us;
    if (pcu_timer_ptr->timer_us > pcu_timer_ptr->goal_us){
      pcu_timer_ptr->status = PCU_TIMER_STATUS_FINISH;
    }
  }
  else if (pcu_timer_ptr->status == PCU_TIMER_STATUS_FINISH){
    //
  }
  else{
    pcu_timer_ptr->status = PCU_TIMER_STATUS_IDLE;
  }
  return pcu_timer_get_status(pcu_timer_ptr);
}

uint8_t pcu_timer_get_status(type_PCU_Timer* pcu_timer_ptr)
{
  return pcu_timer_ptr->status;
}
