  /**
  ******************************************************************************
  * @file           : dep.c
  * @version        : v1.0
  * @brief          : библиотека для управления модулем DEP, использует объект ВШ (internal_bus), или объект МКО-контроллер шины
  * @author			: Стюф Алексей/Alexe Styuf <a-styuf@yandex.ru>
  * @date			: 2021.10.01
  ******************************************************************************
  */

#include "dep.h"

/**
  * @brief  инициализация структуры управления
	* @param  dep_ptr указатель на структуру управления
	* @param  id номер устройства на внутренней шине
	* @param  device_number номер устройства в котором работает МПП
	* @param  frame_type тип кадра для МПП
	* @param  ib_ptr указатель на внутреннюю шину
	* @param  mko_bc_ptr указатель на контроллер шины МКО
	* @param  gl_fr_num указатель на сквозной глобальный номер кадра
  */
void dep_init(  typeDEPStruct* dep_ptr, 
                uint8_t self_num, 
                uint8_t id, 
                uint16_t device_number, 
                uint16_t frame_type, 
                uint16_t rp_frame_type, 
                typeIBStruct* ib_ptr, 
                typeMKOBCStruct* mko_bc_ptr, 
                uint32_t* gl_fr_num, 
                uint8_t mode, 
				uint32_t offset)
{
	dep_reset_parameters(dep_ptr);
	dep_ptr->id = id;
	dep_ptr->device_number = device_number;
	dep_ptr->frame_type = frame_type;
	dep_ptr->rp_frame_type = rp_frame_type;
	dep_ptr->ib_ptr = ib_ptr;
	dep_ptr->mko_bc_ptr = mko_bc_ptr;
	dep_ptr->self_num = self_num;
	dep_ptr->global_frame_num_ptr = gl_fr_num;
	dep_ptr->mode = mode;

	dep_ptr->offset_to_set = offset;
	dep_ptr->bound_to_set[0] = DEP_RP_WIN_BND_DEFAULT_1;
	dep_ptr->bound_to_set[1] = DEP_RP_WIN_BND_DEFAULT_2;
	dep_ptr->bound_to_set[2] = DEP_RP_WIN_BND_DEFAULT_3;
	// работы с циклограммами
	dep_meas_cycl_init(dep_ptr);
}

/**
  * @brief  инициализация структуры управления
	* @param  dep_ptr указатель на структуру управления
  */
void dep_reset_parameters(typeDEPStruct* dep_ptr)
{
	//зануление данных на передачу в МКО и память
	memset((uint8_t*)&dep_ptr->frame, 0xFE, sizeof(dep_ptr->frame));
	memset((uint8_t*)&dep_ptr->mko_frame, 0xFE, sizeof(dep_ptr->frame));
	memset((uint8_t*)&dep_ptr->dep_ad_frame, 0xFE, sizeof(dep_ptr->dep_ad_frame));
	dep_ptr->frame_data_ready = 0x00;
	// rp data init
	memset((uint8_t*)&dep_ptr->per_rp_frame, 0xFE, sizeof(typeDEP_AD_Peripheral_RP_FrameUnion));
	memset((uint8_t*)&dep_ptr->per_rp_win_frame, 0xFE, sizeof(typeDEP_AD_SELF_RP_Win_FrameUnion));
	memset((uint8_t*)&dep_ptr->rp_frame, 0xFE, sizeof(typeDEP_AD_RP_FrameUnion));

	memset((uint8_t*)&dep_ptr->matrix_val_arr[0], 0xFE, sizeof(dep_ptr->matrix_val_arr));
	memset((uint8_t*)&dep_ptr->matrix_raw, 0x0, sizeof(dep_ptr->matrix_raw));
	memset((uint8_t*)&dep_ptr->win_raw, 0x0, sizeof(dep_ptr->win_raw));
	dep_ptr->rp_polling_ena = 0;
	dep_ptr->dep_max_survey_ena = 0;

	//
	memset((uint8_t*)dep_ptr->rec_fifo, 0xFE, sizeof(dep_ptr->rec_fifo));
	dep_ptr->rec_num = 0;
	dep_ptr->rec_max = 0;
	dep_ptr->mode = DEP_MODE_IB;
	//
	dep_ptr->last_call_time_us = 0;
	dep_ptr->meas_event_num = 0;
	dep_ptr->interval_ms = DEP_DEFAULT_INTERVAL_MS;
	dep_ptr->const_mode = 0x00;
	//
	dep_ptr->current_meas_interval = 0;  // данные для хранения измерительного интервала
  	dep_ptr->rec_num_to_meas_int_change = 0;  // данные для хранения измерительного интервала
	dep_ptr->win_flag_ready = 0;
}

/**
  * @brief  процесс обработки состояния ДЭП
	* @param  dep_ptr указатель на структуру управления
	* @param  time_us время МК us
  */
int8_t dep_process_tp(void* ctrl_struct, uint64_t time_us, typeProcessInterfaceStruct* interface)
{
	uint8_t retval = 0;
	typeFrameStruct frame;
	typeDEPStruct* dep_ptr = (typeDEPStruct*)ctrl_struct;
	/// запуск обработчика по интервалу
	if ((time_us - dep_ptr->last_call_time_us) > (dep_ptr->interval_ms*1000)) {
		dep_ptr->last_call_time_us = time_us;
		// user code begin
		if(*(uint16_t*)&interface->shared_mem[64] != dep_ptr->current_meas_interval){
			dep_ptr->rec_num_to_meas_int_change = dep_ptr->rec_num;
		}
		dep_ptr->current_meas_interval = *(uint16_t*)&interface->shared_mem[64];
				//
		if (dep_ptr->sh_vol == NULL) dep_ptr->sh_vol = (type_DEP_Shared_vol*)&interface->shared_mem[0];
		//
		dep_ptr->current_meas_interval = *(uint16_t*)&interface->shared_mem[64];
		if((cyclo_get_operation_status(&dep_ptr->meas_cyclo) == 0) && (dep_ptr->rp_polling_ena)){
			dep_read_rp_data(dep_ptr);
			dep_rp_matrix_process(dep_ptr);
			// dep data transmit to general block
			if((interface->event[dep_ptr->self_num] & DEP_EVENT_MATRIX_REC_DATA_READY) == 0){
				dep_ptr->sh_vol->fd.matrix_rec = dep_ptr->matrix_raw;
				interface->event[dep_ptr->self_num] |= DEP_EVENT_MATRIX_REC_DATA_READY;
			}
		}
		//
		if (dep_ptr->win_flag_ready){
			dep_ptr->win_flag_ready = 0;
			//
			if((interface->event[dep_ptr->self_num] & DEP_EVENT_WINDOW_REC_DATA_READY) == 0){
				dep_ptr->sh_vol->fd.window_rec = dep_ptr->win_raw;
				interface->event[dep_ptr->self_num] |= DEP_EVENT_WINDOW_REC_DATA_READY;
			}
		}
		// user code end
		retval = 1;
	}
	// обработка event-ов  //todo: сделать специальные функции обработки event-ов
	if (interface->event[dep_ptr->self_num] & DEP_EVENT_MEAS_INTERVAL_START) 
	{
		interface->event[dep_ptr->self_num] &= ~DEP_EVENT_MEAS_INTERVAL_START;
		cyclo_start(&dep_ptr->meas_cyclo);
		dep_ptr->rp_polling_ena = 1;
		dep_ptr->dep_max_survey_ena = 1;
		retval = 1;
	}
	else {
		//
	}
	//обработка циклограммы
	cyclo_handler(&dep_ptr->meas_cyclo, time_us/1000);
	//обработка передачи данных
	if ((interface->event[dep_ptr->self_num] & DEP_EVENT_MEAS_INTERVAL_DATA_READY) == 0){
		if(dep_frame_read_fifo(dep_ptr, &frame))
		{
			memcpy(interface->shared_mem, (uint8_t*)&frame, sizeof(typeFrameStruct));
			interface->event[dep_ptr->self_num] |= DEP_EVENT_MEAS_INTERVAL_DATA_READY;
		}
	}
	return retval;
}

/**
  * @brief  формирование кадра ДЭП c выставлением флага
	* @param  dep_ptr указатель на структуру управления
	* @retval  статус: 0 - кадр не сформирован, 1 - кадр сформирован
  */
int8_t dep_frame_forming(typeDEPStruct* dep_ptr)
{
    uint8_t i = 0;
		typeDEPAcqValue rec_tmp;
		//
		if (dep_ptr->rec_num >= 6){
			dep_ptr->frame.raw.label = 0x0FF1;
			dep_ptr->frame.raw.definer = frame_definer(0, dep_ptr->device_number, NULL, dep_ptr->frame_type);
			dep_ptr->frame.raw.num = ((*dep_ptr->global_frame_num_ptr)++)&0xFFFF;
			dep_ptr->frame.raw.time = _rev_u32_by_u16(clock_get_time_s());
			//
			for (i=0; i<6; i++){
				dep_read_fifo(dep_ptr, &rec_tmp);
				memcpy((uint8_t*)&dep_ptr->frame.dep.meas[i], (uint8_t*)&rec_tmp, sizeof(typeDEPAcqValue));
			}
			//
			dep_ptr->frame.dep.change_int_num = dep_ptr->rec_num_to_meas_int_change;
			dep_ptr->frame.dep.changed_meas_interv = dep_ptr->current_meas_interval;
			//
			dep_ptr->frame.raw.crc16 = frame_crc16((uint8_t*)&dep_ptr->frame.raw, sizeof(typeFrameStruct) - 2);
			//
			dep_mko_frame_forming(dep_ptr);
			dep_frame_write_fifo(dep_ptr, &dep_ptr->frame.raw);
			return 1;
		}
		else{
			return 0;
		}
}

/**
  * @brief  формирование кадра ДЭП для выставления на подадрес МКО
	* @param  dep_ptr указатель на структуру управления
	* @retval  статус: 0 - кадр не сформирован, 1 - кадр сформирован
  */
void dep_mko_frame_forming(typeDEPStruct* dep_ptr)
{
    uint8_t i = 0;
	//
	dep_ptr->mko_frame.raw.label = dep_ptr->frame.raw.label;
	dep_ptr->mko_frame.raw.definer = dep_ptr->frame.raw.definer;
	dep_ptr->mko_frame.raw.num = dep_ptr->frame.raw.num;
	dep_ptr->mko_frame.raw.time = dep_ptr->frame.raw.time;
	//
	for (i=0; i<6; i++){
		dep_ptr->mko_frame.dep.meas[i].Evalue_dep1 = dep_ptr->frame.dep.meas[i].Evalue_dep1;
		dep_ptr->mko_frame.dep.meas[i].Evalue_dep2 = dep_ptr->frame.dep.meas[i].Evalue_dep2;
		dep_ptr->mko_frame.dep.meas[i].Dvalue_dep1 = dep_ptr->frame.dep.meas[i].Dvalue_dep1;
		dep_ptr->mko_frame.dep.meas[i].Dvalue_dep2 = dep_ptr->frame.dep.meas[i].Dvalue_dep2;
		dep_ptr->mko_frame.dep.meas[i].Temp_dep1 = dep_ptr->frame.dep.meas[i].Temp_dep1;
		dep_ptr->mko_frame.dep.meas[i].Temp_dep2 = dep_ptr->frame.dep.meas[i].Temp_dep2;
	}
	//
	dep_ptr->mko_frame.dep.change_int_num = dep_ptr->frame.dep.change_int_num;
	dep_ptr->mko_frame.dep.changed_meas_interv = dep_ptr->frame.dep.changed_meas_interv;
	//
	dep_ptr->mko_frame.raw.crc16 = frame_crc16((uint8_t*)&dep_ptr->mko_frame.raw, sizeof(typeFrameStruct) - 2);
}

/**
  * @brief  включение отключение режима константа (широковещательная)
	* @param  dep_ptr указатель на структуру управления
	* @param  on_off 1 - включение, 0 - отключение
  */
void dep_constant_mode(typeDEPStruct* dep_ptr, uint32_t on_off)
{
	uint16_t data[4] = {0};
	dep_ptr->const_mode = (on_off) ? 0x01 : 0x00;

	if (dep_ptr->mode == DEP_MODE_IB){
		data[0] = __REV16((2 << 8) | (255));
		data[1] = __REV16(dep_ptr->const_mode); 
		ib_run_transaction(dep_ptr->ib_ptr, dep_ptr->id, 16, 0, 2, data);
	}
	else if (dep_ptr->mode == DEP_MODE_MKO){
		data[0] = 0x0006;
		data[1] = 0x0001;
		data[2] = 0x0000;
		data[3] = 0x0000;
		if (mko_bc_transaction_start_with_bus_find(dep_ptr->mko_bc_ptr, MKO_BC_MODE_WRITE, dep_ptr->id, 17, data, 4)){
			//
		}
	}
}

/**
  * @brief  запуск ДЭП командой регистра RUN (4096)
	* @param  dep_ptr указатель на структуру управления
	* @param  time_us время МК us
	* @param  meas_num W - нужное число измерений, R - оставшееся число измерений (+0)
  */
void dep_start(typeDEPStruct *dep_ptr, uint16_t meas_num) 
{
	uint16_t data[4];
	if (dep_ptr->mode == DEP_MODE_IB){
		data[0] = (meas_num);
		ib_run_transaction(dep_ptr->ib_ptr, dep_ptr->id, MB_F_CODE_6, 0x1000, 1, data);
	}
	else if (dep_ptr->mode == DEP_MODE_MKO){
		data[0] = 0x0004;
		data[1] = 0x0002;
		data[2] = 0x0000;
		data[3] = 0x0000;
		if (mko_bc_transaction_start_with_bus_find(dep_ptr->mko_bc_ptr, MKO_BC_MODE_WRITE, dep_ptr->id, 17, data, 4)){
			// 
		}
	}
}

/**
  * @brief  остановка ДЭП командой регистра RUN (4096)
	* @param  dep_ptr указатель на структуру управления
  */
void dep_stop(typeDEPStruct *dep_ptr)
{
	uint16_t data[4];
	if (dep_ptr->mode == DEP_MODE_IB){
		data[0] = (0x0000);
		ib_run_transaction(dep_ptr->ib_ptr, dep_ptr->id, MB_F_CODE_16, 0x1000, 1, data);
	}
	else if (dep_ptr->mode == DEP_MODE_MKO){
		data[0] = 0x0004;
		data[1] = 0x0000;
		data[2] = 0x0000;
		data[3] = 0x0000;
		if (mko_bc_transaction_start_with_bus_find(dep_ptr->mko_bc_ptr, MKO_BC_MODE_WRITE, dep_ptr->id, 17, data, 4)){
			//
		}
	}
}

/**
  * @brief  чтение данных
	* @param  dep_ptr указатель на структуру управления
  */
void dep_read_data(typeDEPStruct *dep_ptr)
{
	uint8_t in_data[64] = {0};
	uint8_t i=0;
	uint16_t received_dep_measurments;
	typeDEPAcqValue tmp_rec;
	volatile typeDepAdFrameUnion dep_ad_frame_raw;
	//
	if (dep_ptr->mode == DEP_MODE_IB){
		if (ib_run_transaction(dep_ptr->ib_ptr, dep_ptr->id, MB_F_CODE_3, 0x2000, 1+4*3, NULL) > 0) { // запрашиваем 3 измерения, что бы почистить буфер
			ib_get_answer_data(dep_ptr->ib_ptr, in_data, 2*(1+4*3));
			received_dep_measurments = __REV16(*(uint16_t*)&in_data[0]);
			//
			if (received_dep_measurments != 0) {
				tmp_rec = *(typeDEPAcqValue*)&in_data[2+i*sizeof(typeDEPAcqValue)];

				dep_ptr->last_meas.Evalue_dep1 = __REV16(tmp_rec.Evalue_dep1);
				dep_ptr->last_meas.Dvalue_dep1 = tmp_rec.Dvalue_dep1;
				dep_ptr->last_meas.Temp_dep1 = tmp_rec.Temp_dep1;
				dep_ptr->last_meas.Evalue_dep2 = __REV16(tmp_rec.Evalue_dep2);
				dep_ptr->last_meas.Dvalue_dep2 = tmp_rec.Dvalue_dep2;
				dep_ptr->last_meas.Temp_dep2 = tmp_rec.Temp_dep2;

				if (dep_write_fifo(dep_ptr, &dep_ptr->last_meas) > 0) {
					dep_ptr->last_field[0] = __dep_field_calculate(dep_ptr->last_meas.Evalue_dep1);
					dep_ptr->last_field[1] = __dep_field_calculate(dep_ptr->last_meas.Evalue_dep2);
					dep_ptr->last_freq[0] = __dep_freq_calculate(dep_ptr->last_meas.Dvalue_dep1);
					dep_ptr->last_freq[1] = __dep_freq_calculate(dep_ptr->last_meas.Dvalue_dep2);
				}
				else{
					//обработка ошибки перезаполениня буфера
				}
			}
		}
		else {
			//todo: возможно необходимо сделать обработку непринятия пакета с данными
		}
	}
	else if (dep_ptr->mode == DEP_MODE_MKO){
		if (mko_bc_transaction_start_with_bus_find(dep_ptr->mko_bc_ptr, MKO_BC_MODE_READ, dep_ptr->id, 2, (uint16_t*)&dep_ad_frame_raw, 32)) {
			memcpy((uint8_t*)&dep_ptr->dep_ad_frame, (uint8_t*)&dep_ad_frame_raw, sizeof(typeDepAdFrameUnion));
		}
		else{
			memset((uint8_t*)&dep_ptr->dep_ad_frame, 0xFE, sizeof(typeDepAdFrameUnion));
		}

		tmp_rec = dep_ad_frame_raw.dep.meas[0];//*(typeDEPAcqValue*)&in_data[2+i*sizeof(typeDEPAcqValue)];

		dep_ptr->last_meas.Evalue_dep1 = tmp_rec.Evalue_dep1;
		dep_ptr->last_meas.Dvalue_dep1 = tmp_rec.Dvalue_dep1;
		dep_ptr->last_meas.Temp_dep1 = tmp_rec.Temp_dep1;
		dep_ptr->last_meas.Evalue_dep2 = tmp_rec.Evalue_dep2;
		dep_ptr->last_meas.Dvalue_dep2 = tmp_rec.Dvalue_dep2;
		dep_ptr->last_meas.Temp_dep2 = tmp_rec.Temp_dep2;

		if (dep_write_fifo(dep_ptr, &dep_ptr->last_meas) > 0) {
			dep_ptr->last_field[0] = __dep_field_calculate(dep_ptr->last_meas.Evalue_dep1);
			dep_ptr->last_field[1] = __dep_field_calculate(dep_ptr->last_meas.Evalue_dep2);
			dep_ptr->last_freq[0] = __dep_freq_calculate(dep_ptr->last_meas.Dvalue_dep1);
			dep_ptr->last_freq[1] = __dep_freq_calculate(dep_ptr->last_meas.Dvalue_dep2);
		}
		else{
			//обработка ошибки перезаполениня буфера
		}
	}
}

/**
  * @brief  чтение данных РП из ДЭП-АД
	* @param  dep_ptr указатель на структуру управления
  */
void dep_read_rp_data(typeDEPStruct *dep_ptr)
{
	volatile typeDEP_AD_Peripheral_RP_FrameUnion dep_rp_frame;
	//
	if (dep_ptr->mode == DEP_MODE_IB){
		// пока такого случая не встречалось
	}
	else if (dep_ptr->mode == DEP_MODE_MKO){
		if (mko_bc_transaction_start_with_bus_find(dep_ptr->mko_bc_ptr, MKO_BC_MODE_READ, dep_ptr->id, 3, (uint16_t*)&dep_rp_frame, 32)) {
			memcpy((uint8_t*)&dep_ptr->per_rp_frame, (uint8_t*)&dep_rp_frame, sizeof(typeDEP_AD_Peripheral_RP_FrameUnion));
		}
		else{
			memset((uint8_t*)&dep_ptr->per_rp_frame, 0xFE, sizeof(typeDEP_AD_Peripheral_RP_FrameUnion));
		}
	}
}

/**
  * @brief  чтение данных окон РП из ДЭП-АД
	* @param  dep_ptr указатель на структуру управления
  */
void dep_read_win_data(typeDEPStruct *dep_ptr)
{
	volatile typeDEP_AD_SELF_RP_Win_FrameUnion per_rp_win_frame;
	//
	if (dep_ptr->mode == DEP_MODE_IB){
		// пока такого случая не встречалось
	}
	else if (dep_ptr->mode == DEP_MODE_MKO){
		if (mko_bc_transaction_start_with_bus_find(dep_ptr->mko_bc_ptr, MKO_BC_MODE_READ, dep_ptr->id, 6, (uint16_t*)&per_rp_win_frame, 32)) {
			memcpy((uint8_t*)&dep_ptr->per_rp_win_frame, (uint8_t*)&per_rp_win_frame, sizeof(typeDEP_AD_SELF_RP_Win_FrameUnion));
		}
		else{
			memset((uint8_t*)&dep_ptr->per_rp_win_frame, 0xFE, sizeof(typeDEP_AD_SELF_RP_Win_FrameUnion));
		}
		dep_ptr->win_flag_ready = 1;
	}
}

/**
  * @brief установка границ окон в РП ДЭП-АД
	* @param  dep_ptr указатель на структуру управления
  */
void dep_bound_set(typeDEPStruct *dep_ptr, uint16_t win_num, uint16_t bound_val)
{
	uint16_t data[4];
	if (dep_ptr->mode == DEP_MODE_IB){
		//
	}
	else if (dep_ptr->mode == DEP_MODE_MKO){
		data[0] = 0x0008;
		data[1] = 0x0000;
		data[2] = win_num;
		data[3] = bound_val;
		if (mko_bc_transaction_start_with_bus_find(dep_ptr->mko_bc_ptr, MKO_BC_MODE_WRITE, dep_ptr->id, 17, data, 4)){
			//
		}
	}
}

/**
  * @brief установка отсечки РП
	* @param  dep_ptr указатель на структуру управления
  */
void dep_rp_offset_set(typeDEPStruct *dep_ptr, uint16_t offset_val)
{
	uint16_t data[4];
	if (dep_ptr->mode == DEP_MODE_IB){
		//
	}
	else if (dep_ptr->mode == DEP_MODE_MKO){
		data[0] = 0x0007;
		data[1] = 0x0000;
		data[2] = offset_val;
		data[3] = 0x0000;
		if (mko_bc_transaction_start_with_bus_find(dep_ptr->mko_bc_ptr, MKO_BC_MODE_WRITE, dep_ptr->id, 17, data, 4)){
			//
		}
	}
}

/**
 * @brief Установка границ окон на отправку
 * 
 * @param dep_ptr 
 * @param bnd_1 
 * @param bnd_2 
 * @param bnd_3 
 * @return int32_t 
 */
int32_t dep_rp_set_win_bnd(typeDEPStruct *dep_ptr, uint8_t bnd_1, uint8_t bnd_2, uint8_t bnd_3)
{
	dep_ptr->bound_to_set[0] = bnd_1;
	dep_ptr->bound_to_set[1] = bnd_2;
	dep_ptr->bound_to_set[2] = bnd_3;
	return 0;
}

/**
 * @brief Construct a new dep rp matrix process object
 * 
 * @param dep_ptr 
 */
void dep_rp_matrix_process(typeDEPStruct *dep_ptr)
{
	uint8_t rec_num = 8;
	if(dep_ptr->per_rp_frame.raw.label == 0x0FF1){
		dep_ptr->matrix_raw.time = dep_ptr->per_rp_frame.raw.time;
		dep_ptr->matrix_raw.amplitude = dep_ptr->per_rp_frame.dep.value;
		dep_ptr->current_offset = dep_ptr->offset_to_set;  //TODO: если в кадре РП появится offset поставить сюда значение из кадра
	}
	else{
		dep_ptr->matrix_raw.time = 0xFEFEFEFE;
		dep_ptr->matrix_raw.amplitude = 0xFEFE;
	}
	//
	if ((dep_ptr->matrix_raw.amplitude != 0xFEFE) && (dep_ptr->matrix_raw.time != 0xFEFEFEFE)){
		for (uint8_t num = 0; num < rec_num; num++){
			if ((dep_ptr->matrix_raw.amplitude > dep_ptr->matrix_val_arr[num].amplitude) || 
										((dep_ptr->matrix_val_arr[num].amplitude == 0xFEFEu) && 
										(dep_ptr->matrix_val_arr[num].time == 0xFEFEFEFEu))){
				memmove((uint8_t*)&dep_ptr->matrix_val_arr[num+1], 
						(uint8_t*)&dep_ptr->matrix_val_arr[num], 
						sizeof(typeDEPADRP_Rec) * (rec_num-1-num));
				dep_ptr->matrix_val_arr[num] = dep_ptr->matrix_raw;
				break;
			}
			else{
				//
			}
		}
	}
}


/**
  * @brief  формирование кадра РП
	* @param  dep_ptr указатель на структуру управления
  */
void dep_rp_frame_forming(typeDEPStruct* dep_ptr)
{
	if ((dep_ptr->matrix_val_arr[0].amplitude != 0xFEFE) && (dep_ptr->matrix_val_arr[0].time != 0xFEFEFEFE)){
		//
		dep_ptr->rp_frame.raw.label = 0x0FF1;
		dep_ptr->rp_frame.raw.definer = frame_definer(0, dep_ptr->device_number, NULL, dep_ptr->rp_frame_type);
		dep_ptr->rp_frame.raw.num = ((*dep_ptr->global_frame_num_ptr)++)&0xFFFF;
		dep_ptr->rp_frame.raw.time = _rev_u32_by_u16(clock_get_time_s());
		//
		memcpy((uint8_t*)dep_ptr->rp_frame.dep.matrix_val, (uint8_t*)dep_ptr->matrix_val_arr, sizeof(dep_ptr->matrix_val_arr));
		memset((uint8_t*)dep_ptr->matrix_val_arr, 0xFE, sizeof(dep_ptr->matrix_val_arr));
		dep_ptr->rp_frame.dep.offset = dep_ptr->current_offset;
		memset((uint8_t*)dep_ptr->rp_frame.dep.reserve, 0xFE, sizeof(dep_ptr->rp_frame.dep.reserve));
		//
		dep_ptr->rp_frame.raw.crc16 = frame_crc16((uint8_t*)&dep_ptr->rp_frame.raw, sizeof(typeFrameStruct) - 2);
		//
		dep_rp_mko_frame_forming(dep_ptr);
		dep_frame_write_fifo(dep_ptr, &dep_ptr->rp_frame.raw);
	}
}


/**
  * @brief  Преобразование кадра к формату МКО
	* @param  dep_ptr указатель на структуру управления
  */
void dep_rp_mko_frame_forming(typeDEPStruct* dep_ptr)
{
		dep_ptr->mko_rp_frame.raw.label = dep_ptr->rp_frame.raw.label;
		dep_ptr->mko_rp_frame.raw.definer = dep_ptr->rp_frame.raw.definer;
		dep_ptr->mko_rp_frame.raw.num = dep_ptr->rp_frame.raw.num;
		dep_ptr->mko_rp_frame.raw.time = dep_ptr->rp_frame.raw.time;
		//
		for(uint8_t num=0; num < 8; num++){
			dep_ptr->mko_rp_frame.dep.matrix_val[num] = dep_ptr->rp_frame.dep.matrix_val[num];
		}
		dep_ptr->mko_rp_frame.dep.offset = dep_ptr->rp_frame.dep.offset;
		memcpy((uint8_t*)dep_ptr->mko_rp_frame.dep.reserve, (uint8_t*)dep_ptr->rp_frame.dep.reserve, sizeof(dep_ptr->rp_frame.dep.reserve));
		//
		dep_ptr->mko_rp_frame.raw.crc16 = frame_crc16((uint8_t*)&dep_ptr->mko_rp_frame.raw, sizeof(typeFrameStruct) - 2);
}


/**
 * @brief подсчет и обновления значение поля, измеренного ДЭП
 * 
 * @param dep_ptr 
 * @param raw_data 
 * @return float 
 */
float __dep_field_calculate(uint16_t raw_data)
{
	return (float)((int16_t)raw_data*10.);
}

/**
 * @brief подсчет и обновления значение частоты, измеренного ДЭП
 * 
 * @param dep_ptr 
 * @param raw_data 
 * @return float 
 */
float __dep_freq_calculate(uint8_t raw_data)
{
	float freq;
	int8_t data = raw_data;
	if (raw_data > 127){
		data = raw_data - 256;
	}
	freq = 5e6 / (33333. + (data * (1<<7)));
	return freq;
}

/**
  * @brief  запись измерения в fifo
	* @param  dep_ptr указатель на структуру управления
	* @param  data указатель на массив с кадром данных
	* @retval  статус записи: 1 - ок, <0 - ошибка
  */
int8_t dep_write_fifo(typeDEPStruct *dep_ptr, typeDEPAcqValue* data)
{
	if (dep_ptr->rec_num >= DEP_REC_FIFO_DEPTH){
		return -1;
	}
	else{
		memcpy((uint8_t*)&dep_ptr->rec_fifo[dep_ptr->rec_num], (uint8_t*)data, sizeof(typeDEPAcqValue));
		dep_ptr->rec_num += 1;
		if (dep_ptr->rec_num > dep_ptr->rec_max) dep_ptr->rec_max = dep_ptr->rec_num;  //сбор информации о заполненности fifo
		return 1;
	}
}

/**
  * @brief  взятие измерения из fifo
	* @param  dep_ptr указатель на структуру управления
	* @param  data указатель на массив с кадром данных
	* @retval  статус: 0 - fifo-пуст, 1 - ок
  */
int8_t dep_read_fifo(typeDEPStruct* dep_ptr, typeDEPAcqValue* data)
{
	if (dep_ptr->rec_num == 0){
		return 0;
	}
	else{
		dep_ptr->rec_num -= 1;
		memcpy((uint8_t*)data, (uint8_t*)&dep_ptr->rec_fifo[0], sizeof(typeDEPAcqValue));
		memmove((uint8_t*)&dep_ptr->rec_fifo[0], (uint8_t*)&dep_ptr->rec_fifo[1], sizeof(typeDEPAcqValue)*dep_ptr->rec_num);
		memset((uint8_t*)&dep_ptr->rec_fifo[dep_ptr->rec_num], 0x00, sizeof(typeDEPAcqValue)*(DEP_REC_FIFO_DEPTH - dep_ptr->rec_num));
		return 1;
	}
}

/**
  * @brief  запись кадра в fifo
	* @param  cm_ptr указатель на структуру управления
	* @param  frame указатель на массив с кадром данных
	* @retval  статус записи: 1 - ок, <0 - ошибка
  */
int8_t dep_frame_write_fifo(typeDEPStruct* dep_ptr, typeFrameStruct* frame)
{
	if (dep_ptr->frames_fifo_num >= DEP_FRAME_FIFO_DEPTH){
		return -1;
	}
	else{
		memcpy((uint8_t*)&dep_ptr->frames_fifo[dep_ptr->frames_fifo_num], (uint8_t*)frame, sizeof(typeFrameStruct));
		dep_ptr->frames_fifo_num += 1;
		if (dep_ptr->frames_fifo_num > dep_ptr->frames_fifo_num_max) dep_ptr->frames_fifo_num_max = dep_ptr->frames_fifo_num;  //сбор информации о заполненности fifo
		return 1;
	}
}

/**
  * @brief  взятие кадра из fifo
	* @param  dep_ptr указатель на структуру управления
	* @param  frame указатель на массив с кадром данных
	* @retval  статус: 0 - fifo-пуст, 1 - ок
  */
int8_t dep_frame_read_fifo(typeDEPStruct* dep_ptr, typeFrameStruct* frame)
{
	if (dep_ptr->frames_fifo_num == 0){
		return 0;
	}
	else{
		dep_ptr->frames_fifo_num -= 1;
		memcpy((uint8_t*)frame, (uint8_t*)&dep_ptr->frames_fifo[0], sizeof(typeFrameStruct));
		memmove((uint8_t*)&dep_ptr->frames_fifo[0], (uint8_t*)&dep_ptr->frames_fifo[1], sizeof(typeFrameStruct)*dep_ptr->frames_fifo_num);
		memset((uint8_t*)&dep_ptr->frames_fifo[dep_ptr->frames_fifo_num], 0x00, sizeof(typeFrameStruct)*(DEP_FRAME_FIFO_DEPTH - dep_ptr->frames_fifo_num));
		return 1;
	}
}

/**
  * @brief  инициализация циклограмм работы с ДЭП
	* @param  dep_ptr указатель на структуру управления
  */
void dep_meas_cycl_init(typeDEPStruct* dep_ptr)
{
	uint8_t data[32] = {0};
	// циклограмма инициализации МПП
	cyclo_init(&dep_ptr->meas_cyclo, "dep");
	//
	*(uint16_t*)(&data[0]) = 2;
	cyclo_add_step(&dep_ptr->meas_cyclo, dep_meas_cycl_struct_start, (void*)dep_ptr, 0, 100, data);
	//
	cyclo_add_step(&dep_ptr->meas_cyclo, dep_meas_cycl_rp_form_data, (void*)dep_ptr, 0, 100, data);
	cyclo_add_step(&dep_ptr->meas_cyclo, dep_meas_cycl_rp_read_win_data, (void*)dep_ptr, 0, 100, data);
	cyclo_add_step(&dep_ptr->meas_cyclo, dep_meas_cycl_rp_set_offset, (void*)dep_ptr, 0, 100, data);
	data[0] = 0;
	cyclo_add_step(&dep_ptr->meas_cyclo, dep_meas_cycl_rp_set_win_bnd, (void*)dep_ptr, 0, 100, data);
	data[0] = 1;
	cyclo_add_step(&dep_ptr->meas_cyclo, dep_meas_cycl_rp_set_win_bnd, (void*)dep_ptr, 0, 100, data);
	data[0] = 2;
	cyclo_add_step(&dep_ptr->meas_cyclo, dep_meas_cycl_rp_set_win_bnd, (void*)dep_ptr, 0, 100, data);

	cyclo_add_step(&dep_ptr->meas_cyclo, dep_meas_cycl_do_nothing, (void*)NULL, 0, 1100, data);
	//
	cyclo_add_step(&dep_ptr->meas_cyclo, dep_meas_cycl_read_data, (void*)dep_ptr, 0, 0, data);
	cyclo_add_step(&dep_ptr->meas_cyclo, dep_meas_cycl_frame_forming, (void*)dep_ptr, 0, 0, data);
}

/**
  * @brief  обертка функция для согласования типов
	* @param  ctrl_struct указатель на структуру управления
  */
int32_t dep_meas_cycl_struct_start(void* ctrl_struct, uint8_t* data)
{
	typeDEPStruct* dep_ptr = (typeDEPStruct*) ctrl_struct;
	uint16_t meas_time_s = *(uint16_t*)&data[0];
	dep_start(dep_ptr, meas_time_s);
	return 0;
}

/**
  * @brief формирование кадра РП
	* @param  ctrl_struct указатель на структуру управления
  */
int32_t dep_meas_cycl_rp_form_data(void* ctrl_struct, uint8_t* data)
{
	typeDEPStruct* dep_ptr = (typeDEPStruct*) ctrl_struct;
	//
	dep_rp_frame_forming(dep_ptr);
	//
	return 0;
}

/**
  * @brief  обертка функция для согласования типов
	* @param  ctrl_struct указатель на структуру управления
  */
int32_t dep_meas_cycl_struct_stop(void* ctrl_struct, uint8_t* data)
{
	typeDEPStruct* dep_ptr = (typeDEPStruct*) ctrl_struct;
	dep_stop(dep_ptr);
	return 0;
}

/**
  * @brief  обертка функция для согласования типов
	* @param  ctrl_struct указатель на структуру управления
  */
int32_t dep_meas_cycl_read_data(void* ctrl_struct, uint8_t* data)
{
	typeDEPStruct* dep_ptr = (typeDEPStruct*) ctrl_struct;
	dep_read_data(dep_ptr);
	return 0;
}

/**
  * @brief  формирование кадра ДЭП
	* @param  ctrl_struct указатель на структуру управления
  */
int32_t dep_meas_cycl_frame_forming(void* ctrl_struct, uint8_t* data)
{
	typeDEPStruct* dep_ptr = (typeDEPStruct*) ctrl_struct;
	dep_frame_forming(dep_ptr);
	return 0;
}

/**
  * @brief  чтение данных окон
	* @param  ctrl_struct указатель на структуру управления
  */
int32_t dep_meas_cycl_rp_read_win_data(void* ctrl_struct, uint8_t* data)
{
	typeDEPStruct* dep_ptr = (typeDEPStruct*) ctrl_struct;
	//
	dep_read_win_data(dep_ptr);
	//
	return 0;
}

/**
  * @brief  установка границ окон
	* @param  ctrl_struct указатель на структуру управления
  */
int32_t dep_meas_cycl_rp_set_win_bnd(void* ctrl_struct, uint8_t* data)
{
	typeDEPStruct* dep_ptr = (typeDEPStruct*) ctrl_struct;
	uint8_t bnd_num = data[0];
	//
	if (bnd_num < 3){
		dep_bound_set(dep_ptr, bnd_num, dep_ptr->bound_to_set[bnd_num]);
	}
	//
	return 0;
}

/**
  * @brief  установка отсечки rp
	* @param  ctrl_struct указатель на структуру управления
  */
int32_t dep_meas_cycl_rp_set_offset(void* ctrl_struct, uint8_t* data)
{
	typeDEPStruct* dep_ptr = (typeDEPStruct*) ctrl_struct;
	//
	dep_rp_offset_set(dep_ptr, dep_ptr->offset_to_set);
	//
	return 0;
}


/**
  * @brief  функция-заглушка
	* @param  ctrl_struct указатель на структуру управления
  */
int32_t dep_meas_cycl_do_nothing(void* ctrl_struct, uint8_t* data)
{
	return 0;
}
