#ifndef _TASK_PLANNER_H_
#define _TASK_PLANNER_H_

#include <stdint.h>
#include <string.h>
#include <stdio.h>

#define TP_PROCESS_PERIOD_MS    (50)  //! значение максимального времени выполнения обработчиков процессов
#define TP_PROCESS_TIMEOUT_MS   (1)   //! таймаут на обработку потоков для обработки команд вне планировщика
#define TP_PROCESS_WARNING_MS   (TP_PROCESS_PERIOD_MS*0.75)  //! значение времени обработки процесса, после которого выставляется предупреждение
#define TP_PROCESS_ERROR_MS     (TP_PROCESS_PERIOD_MS)  //! значение времени обработки процесса, после которого выставляется ошибка

#define TP_PROCESS_MAX_NUM    32  //! максимальное количество процессов для обработки
#define TP_EVENT_MAX_NUM      32  //! максимальное количество event-ов для процесса (используется для передачи событий)
#define TP_SHARED_MEM_VOL_B   8192  //! размер раcшаренной памяти между процессами (используется для передачи данных)

/**
 * @brief результат обработки процесса 
 */
#define TP_PROCESS_ABSENT -1  //! процесс отсутствует
#define TP_PROCESS_IDLE 0     //! процесс в простое
#define TP_PROCESS_WORK 1     //! процесс обрабатывался

/**
 * @brief маски статусов
 */
#define TP_STATUS_WARNING (1<<0)
#define TP_STATUS_ERROR (1<<1)

#pragma pack(push, 2)

/**
  * @brief  структура читаемого вида времени
  */
typedef struct {
  uint64_t full_us;
  uint32_t day;
  uint8_t hour, minute, second;
  uint16_t ms;
} typeTimeStruct;

/**
  * @brief  структура шаблона для интерфейса между процессами
  * @param  self_num  номер собственного процесса
	* @param  event указатель на массив с событиями
	* @param  shared_mem указатель на массив данных с которым может работать данный процесс
  * @param  shared_mem_len указатель на размер массива с которым работает данный процесс
  */
typedef struct {
  uint8_t self_num;
  uint32_t* event;
  uint8_t* shared_mem;
  uint32_t shared_mem_len;
} typeProcessInterfaceStruct;

/**
  * @brief  структура шаблона для процесса, необходимо применять в описании периферии
	* @param  num номер структуры
	* @param  action указатель на функцию управления процессом периферии, <control_struct> - указатель на модель управления периферией
  * @param  process_time время работы данной функции с момента перезагрузки
  * @param  work_percentage процентное время работы от общего времени работы
  */
typedef struct {
  uint8_t num;
  //
  int8_t (*action) (void* control_struct, uint64_t time_us, typeProcessInterfaceStruct* interface);
  void* control_struct_ptr;
  typeProcessInterfaceStruct interface;
  typeTimeStruct process_time;
  float work_percentage;
} typeProcessTemplateStruct;

/**
  * @brief  структура управления планировщиком задач
  */
typedef struct {
  typeProcessTemplateStruct process[TP_PROCESS_MAX_NUM];
  uint32_t event[TP_EVENT_MAX_NUM];
  uint8_t shared_mem[TP_SHARED_MEM_VOL_B];
  // active parameters
  uint32_t active_process_num;
  uint32_t process_num;
  //
  typeTimeStruct time, work_time;
  uint64_t last_call_time_us;
  uint64_t last_call_finish_time_us;
  float work_percentage;
  //
  uint8_t status;
  uint32_t error_counter;
} typeTPStruct;

#pragma pack(pop)

void tp_init(typeTPStruct* tp_ptr);
void __tp_set_default_parameters(typeTPStruct* tp_ptr);
int8_t tp_process_registration(typeTPStruct* tp_ptr, int8_t (*action) (void*, uint64_t, typeProcessInterfaceStruct*), void* control_struct_ptr, uint32_t sh_mem_offset, uint32_t sh_mem_len);
void tp_handler(typeTPStruct* tp_ptr);
int8_t tp_task_run(typeTPStruct* tp_ptr, uint8_t process_num, uint32_t* task_time_us);
void tp_timer_handler(typeTPStruct* tp_ptr, uint32_t tp_handler_step_time_us);
//
void __time_recalculate(typeTimeStruct* time_ptr);
uint8_t __time_str_repr(typeTimeStruct* time_ptr, char* time_string);
//

#endif
