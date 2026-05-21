#ifndef _POWER_MANAGER_H_
#define _POWER_MANAGER_H_


#include "main.h"
#include "power_management_settings.h"
#include "pwr_channel.h"
#include "adc.h"
#include "gpio.h"
#include "cyclogramma.h"
#include "clock.h"

#define PWR_PROCESS_PERIOD      100

#define PWR_CH_HS_NOT_CHANGED   0xFF

/**
  * @brief  состояния обработки команд питания
*/
typedef enum
{
  PWR_CMD_STATE_IDLE = 0,   //! простой
  PWR_CMD_STATE_READY,      //! команда готова к выполнению
  PWR_CMD_STATE_PROCESS     //! команда принята в обработку и ожидает окончания интервала ожидания
} type_PWR_CH_CMD_STATE;

#pragma pack(push, 2)

/**
 * @brief слепок команды, которая передается в буфер управления каналами питания
 * 
 */
typedef union{
  struct{
    uint8_t process_state;    // состояния обработки команды  (type_PWR_CH_CMD_STATE)
    uint8_t num;              // Номер канала, к которому относится команда
    uint8_t state;            // Желаемое состояние канала (например, ВКЛ/ВЫКЛ)
    uint8_t half_set;         // Номер полукомплекта (0 - первый, 1 - второй, 0xFF - не используется). Используется только для каналов с полукомплектом
    uint16_t delay_ms;        // Задержка в миллисекундах перед выполнением команды
  }field;
  uint8_t raw[6];             // Сырой вид команды для прямой манипуляции
} type_PWR_CMD;

#pragma pack(pop)

/**
 * @brief  структура для организации буфера команд управления каналами питания,
 * на один канал - только одна команда, которая перезаписывается при поступлении новой команды
 */
typedef struct{
  type_PWR_CMD   cmd_array[PWR_CH_NUMBER];    // массив команд по одной на канал
  type_PWR_CMD   last_cmd;                    // последняя команда, которая была обработана
  uint32_t  cmd_rem;                          // количество оставшихся команд в буфере
  uint32_t  cmd_cnt;                          // общее полное количество команд, переданных в буфер
  uint32_t  cmd_lost;                         // количество потерянных команд из-за перетирания новыми командами
  uint8_t   last_cmd_num;
} type_PWR_CMD_BUFFER;


/**
  * @brief  структура управления каналом питания
  */
typedef struct
{
  // структуры управления отдельными каналами
  typePowerCh ch[PWR_CH_NUMBER];
  uint32_t global_busy;
  // отчетные структуры
  uint32_t status;
  uint32_t state;
  uint32_t half_set;
  // состояния по умолчанию для применения после инициализации
  uint8_t def_state[PWR_CH_NUMBER];
  uint8_t def_hs[PWR_CH_NUMBER];
  uint16_t def_delay[PWR_CH_NUMBER];
  // отчет по потреблению
  float curr_report_fp[PWR_CH_NUMBER];
  // организация очереди команд
  type_PWR_CMD_BUFFER cmd_buffer;
  uint8_t initialisation_flag;
  uint32_t initialisation_timeout_ms;
  uint8_t gap;
  //
  uint32_t error_cnter;
  //
  uint64_t call_interval_us;
  uint64_t last_call_time_us;
}typePower;


//
void pwr_init(typePower* pwr_ptr, typeADCStruct* adc_ptr, type_GPIO_OAI_cm *io_ptr);
// task planer function
int8_t pwr_process_tp(void* ctrl_struct, uint64_t time_us, typeProcessInterfaceStruct* interface);
//
void pwr_step_process(typePower* pwr_ptr, uint32_t interval_ms);
void pwr_synchronize_state_and_status(typePower* pwr_ptr);
void pwr_on_off_by_num(typePower* pwr_ptr, uint8_t num, uint8_t state);
void pwr_status_reset_by_num(typePower* pwr_ptr, uint8_t num);
void pwr_set_state(typePower* pwr_ptr, uint32_t state);
void pwr_set_bound(typePower* pwr_ptr, uint8_t num, uint16_t bound);
void pwr_create_report(typePower* pwr_ptr);
void pwr_change_default_state(typePower* pwr_ptr, uint32_t state);
void pwr_update_default_state(typePower* pwr_ptr);
void pwr_set_default(typePower* pwr_ptr);
//
uint8_t pwr_queue_get_cmd(typePower* pwr_ptr, type_PWR_CMD *cmd);
void pwr_queue_put_cmd(typePower* pwr_ptr, uint16_t delay_ms, uint8_t pwr_ch_num, uint8_t state, uint8_t half_set);
// static
void __pwr_calc_current_coefficients(float r_sh, float r_fb, float* curr_a_ptr, float* curr_b_ptr);
//
int8_t pwr_buffer_init(typePower* pwr_ptr);
int8_t pwr_buffer_write(typePower* pwr_ptr, type_PWR_CMD cmd);
type_PWR_CMD pwr_buffer_read(typePower* pwr_ptr);
int32_t pwr_buffer_process(typePower* pwr_ptr, uint32_t interval_ms);

#endif
