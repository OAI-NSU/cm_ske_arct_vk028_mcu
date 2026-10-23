  /**
  ******************************************************************************
  * @file           : mkkd.c
  * @version        : v1.0
  * @brief          : библиотека для управления модулем MKKD, использует объект ВШ (internal_bus), или объект МКО-контроллер шины
  * @author			: Стюф Алексей/Alexe Styuf <a-styuf@yandex.ru>
  * @date			: 2021.10.01
  ******************************************************************************
  */

#include "mkkd.h"

/**
  * @brief  инициализация структуры управления
	* @param  mkkd_ptr указатель на структуру управления
	* @param  id номер устройства на внутренней шине/адрес МКО
	* @param  device_number номер устройства в котором работает МПП
	* @param  frame_type тип кадра для МПП
	* @param  mko_bc_ptr указатель на контроллер шины МКО
	* @param  gl_fr_num указатель на сквозной глобальный номер кадра
  */
void mkkd_init(  	typeMKKDStruct* mkkd_ptr, 
					uint8_t self_num, 
					uint8_t id, 
					uint16_t device_number, 
					uint16_t frame_type, 
					typeMKOBCStruct* mko_bc_ptr, 
					uint32_t* gl_fr_num)
{
	mkkd_reset_parameters(mkkd_ptr);
	mkkd_ptr->id = id;
	mkkd_ptr->device_number = device_number;
	mkkd_ptr->frame_type = frame_type;
	mkkd_ptr->mko_bc_ptr = mko_bc_ptr;
	mkkd_ptr->self_num = self_num;
	mkkd_ptr->global_frame_num_ptr = gl_fr_num;
	// работы с циклограммами
	mkkd_meas_cycl_init(mkkd_ptr);
}

/**
  * @brief  инициализация структуры управления
	* @param  mkkd_ptr указатель на структуру управления
  */
void mkkd_reset_parameters(typeMKKDStruct* mkkd_ptr)
{
	// зануление данных на передачу в МКО и память
	memset((uint8_t*)&mkkd_ptr->frame[0], 0xFE, sizeof(mkkd_ptr->frame));
	memset((uint8_t*)&mkkd_ptr->frame[1], 0xFE, sizeof(mkkd_ptr->frame));
	memset((uint8_t*)&mkkd_ptr->fifo, 0x00, sizeof(typeFrameStruct)*MKKD_REC_FIFO_DEPTH);
	mkkd_ptr->frame_data_ready = 0x00;
	//
	mkkd_ptr->last_call_time_us = 0;
	mkkd_ptr->call_interval_us = 0;
	mkkd_ptr->meas_event_num = 0;
	mkkd_ptr->interval_ms = MKKD_DEFAULT_INTERVAL_MS;
	mkkd_ptr->const_mode = 0x00;
}

/**
  * @brief  процесс обработки состояния ДЭП
	* @param  mkkd_ptr указатель на структуру управления
	* @param  time_us время МК us
  */
int8_t mkkd_process_tp(void* ctrl_struct, uint64_t time_us, typeProcessInterfaceStruct* interface)
{
	uint8_t retval = 0;
	typeMKKDStruct* mkkd_ptr = (typeMKKDStruct*)ctrl_struct;
	typeFrameStruct frame;
	/// запуск обработчика по интервалу
	if ((time_us - mkkd_ptr->last_call_time_us) > (mkkd_ptr->interval_ms*1000)) {
		mkkd_ptr->call_interval_us = time_us - mkkd_ptr->last_call_time_us;
		mkkd_ptr->last_call_time_us = time_us;
		// user code begin

		// user code end
		retval = 1;
	}
	// обработка event-ов  //todo: сделать специальные фукнции обработки event-ов
	if (interface->event[mkkd_ptr->self_num] & MKKD_EVENT_MEAS_INTERVAL_START) 
	{
		interface->event[mkkd_ptr->self_num] &= ~MKKD_EVENT_MEAS_INTERVAL_START;
		cyclo_start(&mkkd_ptr->meas_cyclo);
		retval = 1;
	}
	else {
		//
	}
	//обработка циклограммы
	cyclo_handler(&mkkd_ptr->meas_cyclo, time_us/1000);
	//обработка передачи данных
	if(mkkd_read_fifo(mkkd_ptr, &frame))
	{
		memcpy(interface->shared_mem, (uint8_t*)&frame, sizeof(typeFrameStruct));
		interface->event[mkkd_ptr->self_num] |= MKKD_EVENT_MEAS_INTERVAL_DATA_READY;
	}
	return retval;
}

/**
  * @brief  включение отключение режима константа (широковещательная)
	* @param  mkkd_ptr указатель на структуру управления
	* @param  on_off 1 - включение, 0 - отключение
  */
void mkkd_constant_mode(typeMKKDStruct* mkkd_ptr, uint32_t on_off)
{
	uint16_t data[4] = {0};
	mkkd_ptr->const_mode = (on_off & 0x01) ? 0x01 : 0x00;

	data[0] = 0x0006;
	data[1] = (on_off) ? 0x0001 : 0x0000;
	data[2] = 0x0000;
	data[3] = 0x0000;
	if (mko_bc_transaction_start_with_bus_find(mkkd_ptr->mko_bc_ptr, MKO_BC_MODE_WRITE, mkkd_ptr->id, 17, data, 4)){
		//
	}
}

/**
  * @brief  запуск измерения МККД
	* @param  mkkd_ptr указатель на структуру управления
  */
void mkkd_start(typeMKKDStruct *mkkd_ptr) 
{
	uint16_t data[4];

	data[0] = 0x0005;
	data[1] = 0x0001;
	data[2] = 0x0000;
	data[3] = 0x0000;
	if (mko_bc_transaction_start_with_bus_find(mkkd_ptr->mko_bc_ptr, MKO_BC_MODE_WRITE, mkkd_ptr->id, 17, data, 4)){
		// 
	}
}

/**
  * @brief  синхронизация времени МККД
	* @param  mkkd_ptr указатель на структуру управления
  */
void mkkd_synch_time(typeMKKDStruct *mkkd_ptr) 
{
	uint16_t data[4];
	uint32_t time_s = clock_get_time_s();

	data[0] = 0x002E;
	data[1] = (time_s >> 16) & 0xFFFF;
	data[2] = (time_s >> 0) & 0xFFFF;
	data[3] = 0x0000;
	if (mko_bc_transaction_start_with_bus_find(mkkd_ptr->mko_bc_ptr, MKO_BC_MODE_WRITE, mkkd_ptr->id, 29, data, 4)){
		// 
	}
}

/**
  * @brief  чтение данных
	* @param  mkkd_ptr указатель на структуру управления
  */
void mkkd_read_data(typeMKKDStruct *mkkd_ptr)
{
	typeFrameStruct frame[2];
	//
	if (mko_bc_transaction_start_with_bus_find(mkkd_ptr->mko_bc_ptr, MKO_BC_MODE_READ, mkkd_ptr->id, 4, (uint16_t*)&frame[0], 32)) {
		memcpy((uint8_t*)&mkkd_ptr->frame[0], (uint8_t*)&frame[0], sizeof(typeMKKD_Self_Frame));
		mkkd_write_fifo(mkkd_ptr, &frame[0]);
	}
	else{
		memset((uint8_t*)&mkkd_ptr->frame[0], 0xFE, sizeof(typeMKKD_Self_Frame));
	}
	//
	if (mko_bc_transaction_start_with_bus_find(mkkd_ptr->mko_bc_ptr, MKO_BC_MODE_READ, mkkd_ptr->id, 5, (uint16_t*)&frame[1], 32)) {
		memcpy((uint8_t*)&mkkd_ptr->frame[1], (uint8_t*)&frame[1], sizeof(typeMKKD_Self_Frame));
		mkkd_write_fifo(mkkd_ptr, &frame[1]);
	}
	else{
		memset((uint8_t*)&mkkd_ptr->frame[1], 0xFE, sizeof(typeMKKD_Self_Frame));
	}
}

/**
  * @brief  запись измерения в fifo
	* @param  mkkd_ptr указатель на структуру управления
	* @param  data указатель на массив с кадром данных
	* @retval  статус записи: 1 - ок, <0 - ошибка
  */
int8_t mkkd_write_fifo(typeMKKDStruct *mkkd_ptr, typeFrameStruct* data)
{
	if (mkkd_ptr->rec_num >= MKKD_REC_FIFO_DEPTH){
		return -1;
	}
	else{
		memcpy((uint8_t*)&mkkd_ptr->fifo[mkkd_ptr->rec_num], (uint8_t*)data, sizeof(typeFrameStruct));
		mkkd_ptr->rec_num += 1;
		if (mkkd_ptr->rec_num > mkkd_ptr->rec_max) mkkd_ptr->rec_max = mkkd_ptr->rec_num;  //сбор информации о заполненности fifo
		return 1;
	}
}

/**
  * @brief  взятие измерения из fifo
	* @param  mkkd_ptr указатель на структуру управления
	* @param  data указатель на массив с кадром данных
	* @retval  статус: 0 - fifo-пуст, 1 - ок
  */
int8_t mkkd_read_fifo(typeMKKDStruct* mkkd_ptr, typeFrameStruct* data)
{
	if (mkkd_ptr->rec_num == 0){
		return 0;
	}
	else{
		mkkd_ptr->rec_num -= 1;
		memcpy((uint8_t*)data, (uint8_t*)&mkkd_ptr->fifo[0], sizeof(typeFrameStruct));
		memmove((uint8_t*)&mkkd_ptr->fifo[0], (uint8_t*)&mkkd_ptr->fifo[1], sizeof(typeFrameStruct)*mkkd_ptr->rec_num);
		memset((uint8_t*)&mkkd_ptr->fifo[mkkd_ptr->rec_num], 0x00, sizeof(typeFrameStruct)*(MKKD_REC_FIFO_DEPTH - mkkd_ptr->rec_num));
		return 1;
	}
}

/**
  * @brief  инициализация циклограмм работы с ДЭП
	* @param  mkkd_ptr указатель на структуру управления
  */
void mkkd_meas_cycl_init(typeMKKDStruct* mkkd_ptr)
{
	uint8_t data[32] = {0};
	//
	cyclo_init(&mkkd_ptr->meas_cyclo, "mkkd");
	//
	*(uint16_t*)(&data[0]) = 2;
	cyclo_add_step(&mkkd_ptr->meas_cyclo, mkkd_meas_cycl_synch_time, (void*)mkkd_ptr, 0, 100, data);
	cyclo_add_step(&mkkd_ptr->meas_cyclo, mkkd_meas_cycl_start, (void*)mkkd_ptr, 0, 500, data);
	cyclo_add_step(&mkkd_ptr->meas_cyclo, mkkd_meas_cycl_read_data, (void*)mkkd_ptr, 0, 0, data);
}

/**
  * @brief  обертка функция для согласования типов
	* @param  ctrl_struct указатель на структуру управления
  */
int32_t mkkd_meas_cycl_start(void* ctrl_struct, uint8_t* data)
{
	typeMKKDStruct* mkkd_ptr = (typeMKKDStruct*) ctrl_struct;
	mkkd_start(mkkd_ptr);
	return 0;
}

/**
  * @brief  обертка функция для согласования типов
	* @param  ctrl_struct указатель на структуру управления
  */
int32_t mkkd_meas_cycl_synch_time(void* ctrl_struct, uint8_t* data)
{
	typeMKKDStruct* mkkd_ptr = (typeMKKDStruct*) ctrl_struct;
	mkkd_synch_time(mkkd_ptr);
	return 0;
}

/**
  * @brief  обертка функция для согласования типов
	* @param  ctrl_struct указатель на структуру управления
  */
int32_t mkkd_meas_cycl_read_data(void* ctrl_struct, uint8_t* data)
{
	typeMKKDStruct* mkkd_ptr = (typeMKKDStruct*) ctrl_struct;
	mkkd_read_data(mkkd_ptr);
	return 0;
}
