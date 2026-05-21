#ifndef _PWR_CHANNEL_H_
#define _PWR_CHANNEL_H_

#include <math.h>
#include "main.h"
#include "adc.h"
#include "gpio.h"
#include "timers.h"

/**
 * @brief типы каналов по способу включения/отключения
 */

#define PWR_CH_CTRL_NU        0  //! функция отключения/включения не реализована
#define PWR_CH_FLAG           1  //! функция отключения/включения реализована установкой сигнала в постоянное значение
#define PWR_CH_PULSE          2  //! функция отключения/включения реализована подачей импульса переключения состояния
#define PWR_CH_DV_CTRL        3  //! функция отключения/включения и контроль тока реализован через схему с 4-мя пинами и одним выбором

#define PWR_CH_PULSE_TIME_MS  40  //! длительность импульса включения/отключения для типа канала 2
#define PWR_CH_CTRL_TIME_MS   500  //! длительность импульса включения/отключения для типа канала 3

#define PWR_CH_OFF            0
#define PWR_CH_ON             1


/**
  * @brief  структура переменных, используемых в режиме флага (1 - вкл, 0 - откл или наоборот)
  * при использовании для включения "1" используется нога ena, при использовании для включения "0" - нога inh
  */
typedef struct
{
  uint8_t self_type;
  uint8_t adc_ch_num;
  uint8_t io_ena_num;       //! используются номера, так как заведена общая структура с управление GPIO; 1-вкл, 0-откл
  uint8_t io_inh_num;       //! используются номера, так как заведена общая структура с управление GPIO; 1-откл, 0-вкл
}typePwrChCtrlFlag;

/**
  * @brief  структура переменных, используемых для работы через ЦМ
  */
typedef struct
{
  uint8_t self_type;
  uint8_t adc_ch_num;
  uint8_t io_on_num, io_off_num;  //! используются номера, так как заведена общая структура с управление GPIO
}typePwrChCtrlPulse;

/**
  * @brief  структура переменных, используемых для работы через ВШ
  */
typedef struct
{
  uint8_t self_type;
  uint8_t adc_ch_num;
  uint8_t io_A_num, io_B_num, io_C_num, io_D_num, io_E_num;
}typePwrChCtrlLikeDV;

typedef struct{
  uint32_t current_ms, max_ms;
  uint8_t flag, ready;
} typePwrChTimeoutCtrl;

/**
  * @brief  структура управления каналом питания
  */
typedef struct
{
  // псведоним
  char alias[32];           //! псевдоним канала для разбора и формирования отчетов
  // настройки работы канала
  uint8_t type;           //! тип управления каналом
  uint8_t auto_control;   //! автоматическое отключение по превышению тока
  uint8_t hs_ch_mode;      //! наличие полукомплекта
  //
  uint32_t need_to_update; //! переменная для определения необходимости подачи команд
  // переменные отображения состояния канала
  uint8_t state;      //! состояние канала: 0 - выключен, 1 - включен
  uint8_t status;     //! статус канала: 0 - норма, 1 - ошибка
  uint8_t half_set;   //! состояние каналов с полукомплектом: 0 - первый полукомплект, 1 - второй полукомплект
  uint8_t error_cnt;  
  uint8_t adc_ch_num;
  // структуры для управления каналами питания
  type_GPIO_OAI_cm* io_ptr;  //! указатель на общую структуру управления GPIO используемых в ЦМ
  typeADCStruct* adc_ptr;    //! указатель на общую структуру управления АЦП используемых в ЦМ
  //
  typePwrChCtrlPulse puls_ctrl;
  typePwrChCtrlLikeDV dv_ctrl;
  typePwrChCtrlFlag flag_ctrl;
  //
  typePwrChTimeoutCtrl pulse_timeout, ctrl_timeout;
  // граница тока для срабатывания защиты/сигнализации по току
  float current_bound_mA; 
  // переменные получения параметров канала
  float curr_a, curr_b;   //! коэффициенты пересчета сырых показаний АЦП в значения тока в мА
  float current_fp_mA;    //! значение тока в мА
  uint16_t current_mA;    //! значение тока в мА
}typePowerCh;

//
int8_t pwr_ch_init(typePowerCh* pwr_ch_ptr, const char *alias, uint8_t type, uint8_t auto_control, uint8_t hs_ch_mode, float bound_mA, float A, float B, type_GPIO_OAI_cm *io_ptr, typeADCStruct *adc_ptr, uint8_t* io_cfg, uint8_t* adc_cfg);
uint8_t pwr_ch_process(typePowerCh* pwr_ch_ptr, uint32_t interval_ms);

//
typePwrChCtrlFlag __pwr_ch_flag_setting(uint8_t adc_ch_num, uint8_t io_ena_num, uint8_t io_inh_num);
typePwrChCtrlPulse __pwr_ch_pulse_setting(uint8_t adc_ch_num, uint8_t io_on_num, uint8_t io_off_num);
typePwrChCtrlLikeDV __pwr_ch_dv_like_setting(	uint8_t adc_ch_num, 
											uint8_t io_A_num, uint8_t io_B_num, 
											uint8_t io_C_num, uint8_t io_D_num, 
											uint8_t io_E_num);
//
void pwr_ch_on_off(typePowerCh* pwr_ch_ptr, uint8_t state);

void pwr_ch_half_set_choosing(typePowerCh* pwr_ch_ptr, uint8_t half_set);
void __pwr_ch_ctrl_processor(typePowerCh* pwr_ch_ptr, uint32_t period_ms);
void pwr_ch_dv_like_set_io_msk(typePowerCh* pwr_ch_ptr, uint8_t abcd, uint8_t e);
void _pwr_ch_dv_like_set_on_off(typePowerCh* pwr_ch_ptr, uint8_t mode);
void _pwr_ch_dv_like_set_default(typePowerCh* pwr_ch_ptr);
void _pwr_ch_pulse_set_on_off(typePowerCh* pwr_ch_ptr, uint8_t mode);
void _pwr_ch_pulse_set_default(typePowerCh* pwr_ch_ptr);
void _pwr_ch_flag_set_on_off(typePowerCh* pwr_ch_ptr, uint8_t mode);
void pwr_ch_set_current_bound(typePowerCh* pwr_ch_ptr, float bound);
uint8_t pwr_ch_get_busy(typePowerCh* pwr_ch_ptr);
//
void timeout_init(typePwrChTimeoutCtrl *timeout_ptr, uint32_t max_time);
void timeout_start(typePwrChTimeoutCtrl *timeout_ptr);
void timeout_processor(typePwrChTimeoutCtrl *timeout_ptr, uint32_t interval_ms);
uint8_t timeout_get_state(typePwrChTimeoutCtrl *timeout_ptr);
uint8_t timeout_get_ready(typePwrChTimeoutCtrl *timeout_ptr);


#endif
