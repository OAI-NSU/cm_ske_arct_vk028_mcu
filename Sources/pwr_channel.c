/**
 * @file pwr_channel.c
 * @author Alexey Styuf (a-styuf@yandex.ru)
 * @brief 
 * @version 0.1
 * @date 2022-05-06
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "pwr_channel.h"

/**
 * @brief инициализация отдельного канала управления питанием
 * 
 * @param pwr_ch_ptr указатель на структуру управления отдельным каналом питания
 * @param alias псевдоним
 * @param type типы каналов по способу включения/отключения: NU, FLAG, PULSE, DV_CTRL
 * @param auto_control 1 - канал может отключаться автоматически, 0 - канал отключается только вручную
 * @param hs_ch_mode возможность работы с двумя полукомплектами
 * @param bound_mA граница тока потребления по данному каналу в мА
 * @param A коэффициенты пересчета сырых показаний в значения тока в мА: I[mA] = A*row_data + B
 * @param B коэффициенты пересчета сырых показаний в значения тока в мА: I[mA] = A*row_data + B
 * @param io_cfg список каналов gpio для управления каналом
 * @param adc_cfg список каналов adc для получения информации о канале
 * @return int8_t
 */
int8_t pwr_ch_init(typePowerCh* pwr_ch_ptr, const char *alias, uint8_t type, uint8_t auto_control, uint8_t hs_ch_mode, float bound_mA, float A, float B, type_GPIO_OAI_cm *io_ptr, typeADCStruct *adc_ptr, uint8_t* io_cfg, uint8_t* adc_cfg)
{
	memcpy(pwr_ch_ptr->alias, alias, sizeof(pwr_ch_ptr->alias));
	//
	pwr_ch_ptr->type = type;
	pwr_ch_ptr->auto_control = auto_control;
	pwr_ch_ptr->hs_ch_mode = hs_ch_mode;
	//
	pwr_ch_ptr->state = 0x00;
	pwr_ch_ptr->status = 0x00;
	pwr_ch_ptr->half_set = 0x00;
	pwr_ch_ptr->adc_ch_num = adc_cfg[0];
	//
	pwr_ch_ptr->io_ptr = io_ptr;
	pwr_ch_ptr->adc_ptr = adc_ptr;
	//
	pwr_ch_ptr->current_bound_mA = bound_mA;
	//
	pwr_ch_ptr->curr_a = A;
	pwr_ch_ptr->curr_b = B;
	//
	timeout_init(&pwr_ch_ptr->pulse_timeout, PWR_CH_PULSE_TIME_MS);
	timeout_init(&pwr_ch_ptr->ctrl_timeout, PWR_CH_CTRL_TIME_MS);
	//
	switch(type){
		case PWR_CH_CTRL_NU:
			//
			break;
		case PWR_CH_FLAG:
			pwr_ch_ptr->flag_ctrl = __pwr_ch_flag_setting(adc_cfg[0], io_cfg[0], io_cfg[1]);
			break;
		case PWR_CH_PULSE:
			pwr_ch_ptr->puls_ctrl = __pwr_ch_pulse_setting(adc_cfg[0], io_cfg[0], io_cfg[1]);
			break;
		case PWR_CH_DV_CTRL:
			pwr_ch_ptr->dv_ctrl = __pwr_ch_dv_like_setting(adc_cfg[0], io_cfg[0], io_cfg[1], io_cfg[2], io_cfg[3], io_cfg[4]);
			break;
	}
	//
	// printf("pwr_ch init:<%s>, type<%d>, hs<%d>, I bound<%.3E>\n", pwr_ch_ptr->alias, pwr_ch_ptr->type, pwr_ch_ptr->hs_ch_mode, pwr_ch_ptr->current_bound_mA);
	//
	return 1;
}

/**
	* @brief  для каналов без внутреннего контроля тока установка значения из вне, для каналов с внутренним контролем - обновление данных тока
	* @param  pwr_ch_ptr указатель на структуру управления
	* @retval 1 - ОК, 0 - необходимо отключить канал
	
  */
uint8_t pwr_ch_process(typePowerCh* pwr_ch_ptr, uint32_t interval_ms)
{
	float adc_voltage;
	volatile uint8_t ret_val = 1;
	//
	timeout_processor(&pwr_ch_ptr->ctrl_timeout, interval_ms);
	timeout_processor(&pwr_ch_ptr->pulse_timeout, interval_ms);
	//
	__pwr_ch_ctrl_processor(pwr_ch_ptr, interval_ms);
	//
	adc_voltage = adc_ch_voltage(pwr_ch_ptr->adc_ptr, pwr_ch_ptr->adc_ch_num);
	pwr_ch_ptr->current_fp_mA = pwr_ch_ptr->curr_a * adc_voltage + pwr_ch_ptr->curr_b;
	if (pwr_ch_ptr->current_fp_mA > 0) pwr_ch_ptr->current_mA = (uint16_t)floor(pwr_ch_ptr->current_fp_mA);
	else pwr_ch_ptr->current_mA = 0;
	//
	if (pwr_ch_ptr->current_bound_mA == 0){
		//
	}
	else if(pwr_ch_ptr->current_fp_mA >= pwr_ch_ptr->current_bound_mA){
		pwr_ch_ptr->status = 1;
		if ((pwr_ch_ptr->auto_control) && (pwr_ch_ptr->state)) {
			ret_val = 0;
		}
	}
	return ret_val;
}

/**
 * @brief формирование настроек для работы с каналом импульсного воздействия
 * 
 * @param adc_ch_num 
 * @param io_on_num 
 * @param io_off_num 
 * @return typePwrChCtrlPulse 
 */
typePwrChCtrlPulse __pwr_ch_pulse_setting(uint8_t adc_ch_num, uint8_t io_on_num, uint8_t io_off_num)
{
	typePwrChCtrlPulse retcfg;
	retcfg.self_type = PWR_CH_PULSE;
	retcfg.adc_ch_num = adc_ch_num;
	retcfg.io_on_num = io_on_num;
	retcfg.io_off_num = io_off_num;
	return retcfg;
}

/**
 * @brief формирование настроек для работы с каналом включения постоянным уровнем
 * 
 * @param adc_ch_num 
 * @param io_ena_num 
 * @param io_inh_num 
 * @return typePwrChCtrlFlag 
 */
typePwrChCtrlFlag __pwr_ch_flag_setting(uint8_t adc_ch_num, uint8_t io_ena_num, uint8_t io_inh_num)
{
	typePwrChCtrlFlag retcfg;
	retcfg.self_type = PWR_CH_FLAG;
	retcfg.adc_ch_num = adc_ch_num;
	retcfg.io_ena_num = io_ena_num;
	retcfg.io_inh_num = io_inh_num;
	return retcfg;
}


/**
 * @brief формирование настроек для управления каналом питания ДВ
 * 
 * @param adc_ch_num 
 * @param io_A_num 
 * @param io_B_num 
 * @param io_C_num 
 * @param io_D_num 
 * @param io_E_num 
 * @return typePwrChCtrlLikeDV 
 */
typePwrChCtrlLikeDV __pwr_ch_dv_like_setting(	uint8_t adc_ch_num, 
											uint8_t io_A_num, uint8_t io_B_num, 
											uint8_t io_C_num, uint8_t io_D_num, 
											uint8_t io_E_num)
{
	typePwrChCtrlLikeDV retcfg;
	retcfg.self_type = PWR_CH_DV_CTRL;
	retcfg.adc_ch_num = adc_ch_num;
	retcfg.io_A_num = io_A_num;
	retcfg.io_B_num = io_B_num;
	retcfg.io_C_num = io_C_num;
	retcfg.io_D_num = io_D_num;
	retcfg.io_E_num = io_E_num;
	return retcfg;
}

/**
  * @brief  включение/отключение канала питания
	* @param  pwr_ch_ptr указатель на структуру управления
	* @param  state  1: on, 0: off
  */
void pwr_ch_on_off(typePowerCh* pwr_ch_ptr, uint8_t state)
{
	pwr_ch_ptr->state = state;
	pwr_ch_ptr->need_to_update = 1;
}

/**
 * @brief выбор полукомплекта управления каналом питания
 * 
 * @param pwr_ch_ptr 
 * @param half_set 
 */
void pwr_ch_half_set_choosing(typePowerCh* pwr_ch_ptr, uint8_t half_set)
{
	if (pwr_ch_ptr->hs_ch_mode)	pwr_ch_ptr->half_set = half_set & 0x01;
	else {};
}

/**
 * @brief управления каналами, с 
 * 
 * @param pwr_ch_ptr указатель на модель управления каналом
 */
void __pwr_ch_ctrl_processor(typePowerCh* pwr_ch_ptr, uint32_t period_ms)
{
	if (pwr_ch_ptr->need_to_update){
		// printf("<%s> cmd: type<%d> state<%d>\n", pwr_ch_ptr->alias, pwr_ch_ptr->type, pwr_ch_ptr->state);
		//
		pwr_ch_ptr->need_to_update = 0;
		switch(pwr_ch_ptr->type){
			case PWR_CH_CTRL_NU:
				//
			break;
			case PWR_CH_FLAG:
				_pwr_ch_flag_set_on_off(pwr_ch_ptr, pwr_ch_ptr->state);
			break;
			case PWR_CH_PULSE:
				_pwr_ch_pulse_set_on_off(pwr_ch_ptr, pwr_ch_ptr->state);
				timeout_start(&pwr_ch_ptr->pulse_timeout);
				timeout_start(&pwr_ch_ptr->ctrl_timeout);
			break;
			case PWR_CH_DV_CTRL:
				_pwr_ch_dv_like_set_on_off(pwr_ch_ptr, pwr_ch_ptr->state);
				timeout_start(&pwr_ch_ptr->pulse_timeout);
				timeout_start(&pwr_ch_ptr->ctrl_timeout);
			break;
			default:
				pwr_ch_ptr->error_cnt += 1;
			break;
		}
	}
	else if (timeout_get_ready(&pwr_ch_ptr->pulse_timeout)){
		// printf("<%s> finish: type<%d> state<%d>\n", pwr_ch_ptr->alias, pwr_ch_ptr->type, pwr_ch_ptr->state);
		//
		switch(pwr_ch_ptr->type){
			case PWR_CH_CTRL_NU:
				//
			break;
			case PWR_CH_FLAG:
				
			break;
			case PWR_CH_PULSE:
				_pwr_ch_pulse_set_default(pwr_ch_ptr);
			break;
			case PWR_CH_DV_CTRL:
				_pwr_ch_dv_like_set_default(pwr_ch_ptr);
			break;
			default:
				pwr_ch_ptr->error_cnt += 1;
			break;
		}
	}
}

void pwr_ch_dv_like_set_io_msk(typePowerCh* pwr_ch_ptr, uint8_t abcd, uint8_t e)
{
	if (pwr_ch_ptr->type == PWR_CH_DV_CTRL){
		gpio_set(&pwr_ch_ptr->io_ptr->io[pwr_ch_ptr->dv_ctrl.io_A_num], ((abcd >> 0) & 0x01));
		gpio_set(&pwr_ch_ptr->io_ptr->io[pwr_ch_ptr->dv_ctrl.io_B_num], ((abcd >> 1) & 0x01));
		gpio_set(&pwr_ch_ptr->io_ptr->io[pwr_ch_ptr->dv_ctrl.io_C_num], ((abcd >> 2) & 0x01));
		gpio_set(&pwr_ch_ptr->io_ptr->io[pwr_ch_ptr->dv_ctrl.io_D_num], ((abcd >> 3) & 0x01));
		//
		gpio_set(&pwr_ch_ptr->io_ptr->io[pwr_ch_ptr->dv_ctrl.io_E_num], ((e) & 0x01));
	}
	else{
		pwr_ch_ptr->error_cnt += 1;
	}
}

void _pwr_ch_dv_like_set_on_off(typePowerCh* pwr_ch_ptr, uint8_t mode)
{
	uint8_t abcd_io_mask = 0x00;
	if (pwr_ch_ptr->type == PWR_CH_DV_CTRL){
		if ((mode & 0x01) == PWR_CH_ON) abcd_io_mask = (pwr_ch_ptr->half_set == 0) ? 0x09 : 0x06;
		else abcd_io_mask = (pwr_ch_ptr->half_set == 0) ? 0x03 : 0x0C;
		pwr_ch_dv_like_set_io_msk(pwr_ch_ptr, abcd_io_mask, 0);
	}
	else{
		pwr_ch_ptr->error_cnt += 1;
	}
}

void _pwr_ch_dv_like_set_default(typePowerCh* pwr_ch_ptr)
{
	uint8_t abcd_io_mask = 0x00;
	if (pwr_ch_ptr->type == PWR_CH_DV_CTRL){
		abcd_io_mask = 0x00;	
		pwr_ch_dv_like_set_io_msk(pwr_ch_ptr, abcd_io_mask, 1);
	}
	else{
		pwr_ch_ptr->error_cnt += 1;
	}
}

void _pwr_ch_pulse_set_on_off(typePowerCh* pwr_ch_ptr, uint8_t mode)
{
	if (pwr_ch_ptr->type == PWR_CH_PULSE){
		if ((mode & 0x01) == PWR_CH_ON) gpio_set(&pwr_ch_ptr->io_ptr->io[pwr_ch_ptr->puls_ctrl.io_on_num], 1);
		else  gpio_set(&pwr_ch_ptr->io_ptr->io[pwr_ch_ptr->puls_ctrl.io_off_num], 1);
	}
	else{
		pwr_ch_ptr->error_cnt += 1;
	}
}

void _pwr_ch_pulse_set_default(typePowerCh* pwr_ch_ptr)
{
	if (pwr_ch_ptr->type == PWR_CH_PULSE){
		gpio_set(&pwr_ch_ptr->io_ptr->io[pwr_ch_ptr->puls_ctrl.io_on_num], 0);
		gpio_set(&pwr_ch_ptr->io_ptr->io[pwr_ch_ptr->puls_ctrl.io_off_num], 0);
	}
	else{
		pwr_ch_ptr->error_cnt += 1;
	}
}

void _pwr_ch_flag_set_on_off(typePowerCh* pwr_ch_ptr, uint8_t mode)
{
	if (pwr_ch_ptr->type == PWR_CH_FLAG){
		if ((mode & 0x01) == PWR_CH_ON) {
			gpio_set(&pwr_ch_ptr->io_ptr->io[pwr_ch_ptr->flag_ctrl.io_ena_num], 1);
			gpio_set(&pwr_ch_ptr->io_ptr->io[pwr_ch_ptr->flag_ctrl.io_inh_num], 0);
		}
		else  {
			gpio_set(&pwr_ch_ptr->io_ptr->io[pwr_ch_ptr->flag_ctrl.io_ena_num], 0);
			gpio_set(&pwr_ch_ptr->io_ptr->io[pwr_ch_ptr->flag_ctrl.io_inh_num], 1);
		}
	}
	else{
		pwr_ch_ptr->error_cnt += 1;
	}
}

/**
 * @brief Получение занятости блока на поддержку корректной длительности импульса включения/отключения
 * 
 * @param pwr_ch_ptr 
 * @return uint8_t 1 - занят, 0 - свободен
 */
uint8_t pwr_ch_get_busy(typePowerCh* pwr_ch_ptr)
{	uint8_t busy = 0;
	busy = timeout_get_state(&pwr_ch_ptr->pulse_timeout);
	return busy;
}

/**
 * @brief установка границы срабатывания токовой защиты
 * 
 * @param pwr_ch_ptr указатель на структуру управления
 * @param bound значение границы срабатывания токовой защиты в мА
 */
void pwr_ch_set_current_bound(typePowerCh* pwr_ch_ptr, float bound)
{
	pwr_ch_ptr->current_bound_mA = bound;
	pwr_ch_ptr->status = 0;
}

void timeout_init(typePwrChTimeoutCtrl *timeout_ptr, uint32_t max_time)
{
	timeout_ptr->max_ms = max_time;
	timeout_ptr->flag = 0;
	timeout_ptr->ready = 0;
	timeout_ptr->current_ms = 0;
}

void timeout_start(typePwrChTimeoutCtrl *timeout_ptr)
{
	timeout_ptr->flag = 1;
	timeout_ptr->ready = 0;
}

void timeout_processor(typePwrChTimeoutCtrl *timeout_ptr, uint32_t interval_ms)
{
	if (timeout_ptr->flag){
		timeout_ptr->current_ms += interval_ms;
	}
	//
	if (timeout_ptr->current_ms >= timeout_ptr->max_ms){
		timeout_ptr->current_ms = 0;
		timeout_ptr->flag = 0;
		timeout_ptr->ready = 1;
	}
}

uint8_t timeout_get_state(typePwrChTimeoutCtrl *timeout_ptr)
{
	return timeout_ptr->flag;
}

uint8_t timeout_get_ready(typePwrChTimeoutCtrl *timeout_ptr)
{
	if (timeout_ptr->ready){
		timeout_ptr->ready = 0;
		return 1;
	}
	else{
		return 0;
	}
}
