  /**
  ******************************************************************************
  * @file           : cyclogramma.c
  * @version        : v1.0
  * @brief          : библиотека для организации циклограмм опроса периферии
	* @note						: используется на базе планировщика задач, следить, что бы каждый шаг был не более 25 мс
  * @author			    : Стюф Алексей/Alexe Styuf <a-styuf@yandex.ru>
  * @date						: 2021.09.25
  ******************************************************************************
  */

#include "cyclogramma.h"

/**
  * @brief  инициализация структуры управления циклограммой
	* @param  cyclo_ptr указатель на структуру управления
	* @param  name строка с именем циклограммы, для отладки, не более 16 символов (включая терминатор)
  */
void cyclo_init(typeCyclograma* cyclo_ptr, char* name)
{
	memset((uint8_t*)cyclo_ptr, 0x00, sizeof(typeCyclograma));
	strcpy(cyclo_ptr->name, name);
}

/**
  * @brief  добавление шага циклограммы 
	* @param  cyclo_ptr указатель на структуру управления
	* @param  func функция, выполняемая в данном шаге
	* @param  ctrl_struct указатель на структуру управления для передачи в функцию обработки шага
	* @param  repеats количество повторений данного шага (количество выполнений = repeats + 1)
	* @param  delay_ms задержка до следующего шага
	* @retval  количество добавленных шагов всего либо ошибка добавления (-1)
  */
int8_t cyclo_add_step(typeCyclograma* cyclo_ptr, int32_t (*func) (void*, uint8_t*), void* ctrl_struct, uint32_t repeats, uint32_t delay_ms, uint8_t* data)
{
	if (cyclo_ptr->step_number == CYCLO_MAX_STEP) return -1;
	else {
		cyclo_ptr->step[cyclo_ptr->step_number].ctrl_struct = ctrl_struct;
		memcpy(cyclo_ptr->step[cyclo_ptr->step_number].data, data, 32);
		cyclo_ptr->step[cyclo_ptr->step_number].func = func;
		cyclo_ptr->step[cyclo_ptr->step_number].repeat_num = repeats;
		cyclo_ptr->step[cyclo_ptr->step_number].delay_to_next_step_ms = delay_ms;
		//
		cyclo_ptr->step_number += 1;
		return cyclo_ptr->step_number;
	}
}

/**
  * @brief  обработка состояния циклограммы
	* @note  необходимо вызывать чаще, чем расстановленные задержки: временное разрешение работы равно периоду вызова
	* @param  cyclo_ptr указатель на структуру управления
	* @param  time_ms текущее время работы МК в мс
	* @retval  количество добавленных шагов всего либо ошибка добавления (-1)
  */
uint8_t cyclo_handler(typeCyclograma* cyclo_ptr, uint32_t time_ms)
{
	uint32_t call_period = time_ms - cyclo_ptr->last_call_time_ms;
	uint8_t retval = 0;
	switch(cyclo_ptr->mode){
		case (CYCLO_MODE_OFF):
			//
			break;
		case (CYCLO_MODE_PAUSE):
			cyclo_ptr->pause_time += call_period;
			break;
		case (CYCLO_MODE_READY):
			cyclo_ptr->last_step_time = time_ms;
			cyclo_ptr->mode = CYCLO_MODE_WORK;
			break;
		case (CYCLO_MODE_WORK):
			// запускаем функцию-обработчик шага
			if((cyclo_ptr->last_step_duration == 0) || (cyclo_ptr->current_step >= cyclo_ptr->step_number)){
				if(cyclo_ptr->step[cyclo_ptr->current_step].func != 0) {
					cyclo_ptr->func_ret_val = cyclo_ptr->step[cyclo_ptr->current_step].func(cyclo_ptr->step[cyclo_ptr->current_step].ctrl_struct, cyclo_ptr->step[cyclo_ptr->current_step].data);
					if (cyclo_ptr->func_ret_val == 0){ // прерывание повтора
						cyclo_ptr->repeat_cnt = cyclo_ptr->step[cyclo_ptr->current_step].repeat_num;
					}
					else if (cyclo_ptr->func_ret_val == -1){  // прерывание циклограммы
						cyclo_stop(cyclo_ptr);
					}
					cyclo_ptr->last_step_duration += 1;
					retval = 1;
				}
				else{
					cyclo_stop(cyclo_ptr);
					break;
				}
			}
			// обрабатываем задержки между шагами
			if(cyclo_ptr->last_step_duration >= cyclo_ptr->step[cyclo_ptr->current_step].delay_to_next_step_ms) {
				if(cyclo_ptr->repeat_cnt >= cyclo_ptr->step[cyclo_ptr->current_step].repeat_num){
					cyclo_ptr->current_step += 1;
					cyclo_ptr->repeat_cnt = 0;
					cyclo_ptr->last_step_duration = 0;
				}
				else if(cyclo_ptr->repeat_cnt < cyclo_ptr->step[cyclo_ptr->current_step].repeat_num){
					cyclo_ptr->repeat_cnt += 1;
					cyclo_ptr->last_step_duration = 0;
				}
			}
			else {
				cyclo_ptr->last_step_duration += (time_ms - cyclo_ptr->last_step_time);
			}
			cyclo_ptr->last_step_time = time_ms;
			break;
	}
	//
	cyclo_ptr->last_call_time_ms = time_ms;
	//
	return retval;
}

/**
  * @brief  запуск циклограммы или возвращение из паузы
	* @param  cyclo_ptr указатель на структуру управления
	* @param  cyclo_ptr указатель на структуру управления
	* @retval 0 - цикл уже запущен, 1 - цикл запустился, -1 - ошибка
  */
int8_t cyclo_start(typeCyclograma* cyclo_ptr)
{
	switch(cyclo_ptr->mode){
		case (CYCLO_MODE_OFF):
			cyclo_ptr->mode = CYCLO_MODE_READY;
			cyclo_ptr->current_step = 0;
			cyclo_ptr->repeat_cnt = 0;
			return 1;
		case (CYCLO_MODE_PAUSE):
			cyclo_ptr->mode = CYCLO_MODE_WORK;
			cyclo_ptr->last_step_time += cyclo_ptr->pause_time;
			return 0;
		default:
			return -1;
	}
}

/**
  * @brief  остановка циклограммы
	* @param  cyclo_ptr указатель на структуру управления
  */
void cyclo_stop(typeCyclograma* cyclo_ptr)
{
	cyclo_ptr->mode = CYCLO_MODE_OFF;
	cyclo_ptr->current_step = 0;
	cyclo_ptr->repeat_cnt = 0;
	cyclo_ptr->last_step_time = 0;
	cyclo_ptr->last_step_duration = 0;
	cyclo_ptr->pause_time = 0;
}

/**
  * @brief  пауза циклограммы
	* @param  cyclo_ptr указатель на структуру управления
  */
void cyclo_pause(typeCyclograma* cyclo_ptr)
{
	cyclo_ptr->mode = CYCLO_MODE_PAUSE;
	cyclo_ptr->pause_time = 0;
}

/**
 * @brief возвращает состояние работы циклограммы
 * 
 * @param cyclo_ptr 
 * @return uint8_t 
 */
uint8_t cyclo_get_operation_status(typeCyclograma* cyclo_ptr)
{
	if(cyclo_ptr->mode == CYCLO_MODE_OFF) return 0;
	return 1;
}

/**
  * @brief  функция заглушка, которая делает ничего
	* @param  cyclo_ptr указатель на структуру управления
  */
int32_t cyclo_do_nothing(void* ctrl_struct, uint8_t *data)
{
	return 0;
}
