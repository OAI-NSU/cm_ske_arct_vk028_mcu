/**
  ******************************************************************************
  * @file           : bdd.c
  * @version        : v1.0
  * @brief          : библиотека для управления модулем BDD, использует объект MKO в режиме контроллера канала
  * @author			    : Стюф Алексей/Alexe Styuf <a-styuf@yandex.ru>
  * @date						: 2021.10.01
  ******************************************************************************
  */

#include "bdd.h"

/**
  * @brief  инициализация структуры управления
	* @param  bdd_ptr указатель на структуру управления
	* @param  mko_addr адресс устройства БДД
	* @param  device_number номер устройства в котором работает БДД
	* @param  frame_type тип кадра для БДД
	* @param  mko_bc_ptr указатель на структуру управления МКО для БДД
	* @param  gl_fr_num указатель на сковозной глобальный номер кадра
  */
void bdd_init(typeBDDStruct* bdd_ptr, uint8_t self_num, uint8_t mko_addr, uint16_t device_number, uint16_t frame_type, typeMKOBCStruct* mko_bc_ptr, uint8_t mko_bus, uint32_t* gl_fr_num)
{
	//init mko_bc
	//
	bdd_reset_parameters(bdd_ptr);
	bdd_ptr->mko_addr = mko_addr;
	bdd_ptr->mko_bus = mko_bus;
	bdd_ptr->device_number = device_number;
	bdd_ptr->frame_type = frame_type;
	bdd_ptr->mko_bc_ptr = mko_bc_ptr;
	bdd_ptr->self_num = self_num;
	bdd_ptr->global_frame_num_ptr = gl_fr_num;
	// включение БДД
	bdd_on_off(bdd_ptr, 1);
	// работы с циклограммами
	bdd_meas_cycl_init(bdd_ptr);
}

/**
  * @brief  инициализация структуры управления
	* @param  bdd_ptr указатель на структуру управления
  */
void bdd_reset_parameters(typeBDDStruct* bdd_ptr)
{
	// обнуляем кадр
	memset((uint8_t*)&bdd_ptr->frame, 0xFE, sizeof(bdd_ptr->frame));
	//
	bdd_ptr->frame_data_ready = 0x00;
	//
	memset((uint8_t*)bdd_ptr->rec_fifo, 0xFE, sizeof(bdd_ptr->rec_fifo));
	bdd_ptr->rec_num = 0;
	bdd_ptr->rec_max = 0;
	//
	bdd_ptr->last_call_time_us = 0;
	bdd_ptr->meas_event_num = 0;
	bdd_ptr->interval_ms = BDD_DEFAULT_INTERVAL_MS;
	bdd_ptr->const_mode = 0;
}

/**
  * @brief  процесс обработки состояния
	* @param  bdd_ptr указатель на структуру управления
	* @param  time_us время МК us
  */
int8_t bdd_process_tp(void* ctrl_struct, uint64_t time_us, typeProcessInterfaceStruct* interface)
{
	uint8_t retval = 0;
	typeBDDStruct* bdd_ptr = (typeBDDStruct*)ctrl_struct;
	/// запуск обработчика по интервалу
	if ((time_us - bdd_ptr->last_call_time_us) > (bdd_ptr->interval_ms*1000)) {
		bdd_ptr->last_call_time_us = time_us;
		// user code begin
		
		// user code end
		retval = 1;
	}
	// обработка event-ов  //todo: сделать специальные фукнции обработки event-ов
	if (interface->event[bdd_ptr->self_num] & BDD_EVENT_MEAS_INTERVAL_START) 
	{
		interface->event[bdd_ptr->self_num] &= ~BDD_EVENT_MEAS_INTERVAL_START;
		cyclo_start(&bdd_ptr->meas_cyclo);
		retval = 1;
	}
	else {
		//
	}
	//обработка циклограммы
	cyclo_handler(&bdd_ptr->meas_cyclo, time_us/1000);
	//обработка передачи данных
	if(bdd_ptr->frame_data_ready)
	{
		bdd_ptr->frame_data_ready = 0;
		memcpy(interface->shared_mem, (uint8_t*)&bdd_ptr->frame, sizeof(typeBDDFrameUnion));
		interface->event[bdd_ptr->self_num] |= BDD_EVENT_MEAS_INTERVAL_DATA_READY;
	}
	return retval;
}

/**
  * @brief  формирование кадра БДД c выставлением флага
	* @param  bdd_ptr указатель на структуру управления
	* @retval  статус: 0 - кадр не сформирован, 1 - кадр сформирован
  */
int8_t bdd_frame_forming(typeBDDStruct* bdd_ptr)
{
    uint8_t i = 0;
		typeBDDAcqValue rec_tmp;
		//
		if (bdd_ptr->rec_num >= BDD_MEAS_NUMBER){
			bdd_ptr->frame.raw.label = 0x0FF1;
			bdd_ptr->frame.raw.definer = frame_definer(0, bdd_ptr->device_number, NULL, bdd_ptr->frame_type);
			bdd_ptr->frame.raw.num = ((*bdd_ptr->global_frame_num_ptr)++)&0xFFFF;
			bdd_ptr->frame.raw.time = _rev_u32_by_u16(clock_get_time_s());
			//
			memset((uint8_t*)&bdd_ptr->frame.raw.data[0], 0xFEFE, sizeof(bdd_ptr->frame.raw.data));
			//
			for (i=0; i<BDD_MEAS_NUMBER; i++){
				bdd_read_fifo(bdd_ptr, &rec_tmp);
				memcpy((uint8_t*)&bdd_ptr->frame.bdd.meas[i], (uint8_t*)&rec_tmp, sizeof(typeBDDAcqValue));
			}
			//
			bdd_ptr->frame.raw.crc16 = frame_crc16((uint8_t*)&bdd_ptr->frame.raw, sizeof(typeFrameStruct) - 2);
			//
			bdd_ptr->frame_data_ready = 1;
			return 1;
		}
		else{
			return 0;
		}
}

/**
  * @brief  включение режима констант (не используется для БДД)
	* @param  bdd_ptr указатель на структуру управления
	* @param  on_off 1 - включение, 0 - отключение
  */
void bdd_constant_mode(typeBDDStruct* bdd_ptr, uint32_t on_off)
{
	uint16_t ctrl_data[32] = {0};
	// формирование команды
	ctrl_data[0] = 0x0FF1;
	ctrl_data[5] = BDD_MK_COMMAND_CONSTANT_MODE;
	ctrl_data[6] = (on_off & 0x01) ? 0x0001 : 0x0000;
	//....
	mko_bc_set_bus(bdd_ptr->mko_bc_ptr, bdd_ptr->mko_bus);
	mko_bc_transaction_start(bdd_ptr->mko_bc_ptr, MKO_BC_MODE_WRITE, bdd_ptr->mko_addr, BDD_MK_FRAME_SADDR_COMMAND, ctrl_data, 32);
}

/**
  * @brief  установка указателя чтения БДД
	* @param  bdd_ptr указатель на структуру управления
	* @param  bdd_rd_ptr установка указателя чтения архивной памяти
  */
void bdd_set_rd_ptr(typeBDDStruct* bdd_ptr, uint32_t bdd_rd_ptr)
{
	uint16_t ctrl_data[32] = {0};
	// формирование команды
	ctrl_data[0] = 0x0FF1;
	ctrl_data[5] = BDD_MK_COMMAND_ARCH_MEM_SET_RD_PTR;
	ctrl_data[6] = (bdd_rd_ptr >> 16) & 0xFFFF;
	ctrl_data[7] = bdd_rd_ptr & 0xFFFF;
	//....
	mko_bc_set_bus(bdd_ptr->mko_bc_ptr, bdd_ptr->mko_bus);
	mko_bc_transaction_start(bdd_ptr->mko_bc_ptr, MKO_BC_MODE_WRITE, bdd_ptr->mko_addr, BDD_MK_FRAME_SADDR_COMMAND, ctrl_data, 32);
}

/**
  * @brief  включить отключить БДД
	* @param  bdd_ptr указатель на структуру управления
	* @param  mode 1 - включение, 0 - отключение
  */
void bdd_on_off(typeBDDStruct* bdd_ptr, uint32_t mode)
{
	uint16_t ctrl_data[32] = {0};
	// формирование команды
	ctrl_data[0] = 0x0FF1;
	ctrl_data[5] = BDD_MK_COMMAND_SET_BDD_MODE;
	ctrl_data[6] = mode & 0xF;
	// запись в подадрес
	mko_bc_set_bus(bdd_ptr->mko_bc_ptr, bdd_ptr->mko_bus);
	mko_bc_transaction_start(bdd_ptr->mko_bc_ptr, MKO_BC_MODE_WRITE, bdd_ptr->mko_addr, BDD_MK_FRAME_SADDR_COMMAND, ctrl_data, 32);
}

/**
  * @brief  калибровка датчика давления ОАИ
	* @param  bdd_ptr указатель на структуру управления
	* @param  mode 1 - атмосфера, 2 - 10^-5, 0xDEFA - значение по умолчанию
  */
void bdd_oai_calibration(typeBDDStruct* bdd_ptr, uint32_t mode)
{
	uint16_t ctrl_data[32] = {0};
	// формирование команды
	ctrl_data[0] = 0x0FF1;
	ctrl_data[5] = BDD_MK_COMMAND_OAI_DD_CALIBRATION;
	ctrl_data[6] = 0xABBA;
	ctrl_data[7] = mode & 0xFFFF;
	// запись в подадрес
	mko_bc_set_bus(bdd_ptr->mko_bc_ptr, bdd_ptr->mko_bus);
	mko_bc_transaction_start(bdd_ptr->mko_bc_ptr, MKO_BC_MODE_WRITE, bdd_ptr->mko_addr, BDD_MK_FRAME_SADDR_COMMAND, ctrl_data, 32);
}

/**
  * @brief  отладочная команда установки STM
	* @param  bdd_ptr указатель на структуру управления
	* @param  mode 1 - атмосфера, 2 - 10^-5, 0xDEFA - значение по умолчанию
  */
void bdd_stm_debug(typeBDDStruct* bdd_ptr, uint32_t value, uint32_t timeout_ms)
{
	uint16_t ctrl_data[32] = {0};
	// формирование команды
	ctrl_data[0] = 0x0FF1;
	ctrl_data[5] = BDD_MK_COMMAND_SET_STM_DEBUG;
	ctrl_data[6] = value & 0xFFFF;
	ctrl_data[7] = timeout_ms & 0xFFFF;
	// запись в подадрес
	mko_bc_set_bus(bdd_ptr->mko_bc_ptr, bdd_ptr->mko_bus);
	mko_bc_transaction_start(bdd_ptr->mko_bc_ptr, MKO_BC_MODE_WRITE, bdd_ptr->mko_addr, BDD_MK_FRAME_SADDR_COMMAND, ctrl_data, 32);
}

/**
  * @brief  инициализация БДД
	* @param  bdd_ptr указатель на структуру управления
  */
void bdd_hw_init(typeBDDStruct* bdd_ptr)
{
	uint16_t ctrl_data[32] = {0};
	// формирование команды
	ctrl_data[0] = 0x0FF1;
	ctrl_data[5] = BDD_MK_COMMAND_SET_INIT_BDD;
	ctrl_data[6] = 0xAA55;
	// запись в подадрес
	mko_bc_set_bus(bdd_ptr->mko_bc_ptr, bdd_ptr->mko_bus);
	mko_bc_transaction_start(bdd_ptr->mko_bc_ptr, MKO_BC_MODE_WRITE, bdd_ptr->mko_addr, BDD_MK_FRAME_SADDR_COMMAND, ctrl_data, 32);
}

/**
  * @brief  чтение измерения БДД
	* @param  bdd_ptr указатель на структуру управления
  */
void bdd_read_sys_frame(typeBDDStruct *bdd_ptr)
{
	typeBDDAcqValue meas;
	mko_bc_set_bus(bdd_ptr->mko_bc_ptr, bdd_ptr->mko_bus);
	mko_bc_transaction_start(bdd_ptr->mko_bc_ptr, MKO_BC_MODE_READ, bdd_ptr->mko_addr, BDD_MK_FRAME_SADDR_SYSTEM, (uint16_t*)&bdd_ptr->sys_frame, 32);
	if ((bdd_ptr->sys_frame.raw.label == 0x0FF1) && ((bdd_ptr->sys_frame.raw.definer & 0xFC07) == 0x4401)){  //0xFC07 - маска проверки определителя БДД без заводского номера
		meas.pressure = bdd_ptr->sys_frame.bdd.pressure;
		meas.temp = bdd_ptr->sys_frame.bdd.temperature;
		meas.hv_current = bdd_ptr->sys_frame.bdd.imd_current;
	}
	else{
		meas.pressure = 0xFEFE;
		meas.temp = 0xFEFE;
		meas.hv_current = 0xFEFE;
	}
	bdd_write_fifo(bdd_ptr, &meas);
}

/**
  * @brief  чтение архивного кадра  БДД
	* @param  bdd_ptr указатель на структуру управления
  */
void bdd_read_arch_frame(typeBDDStruct *bdd_ptr)
{
	mko_bc_set_bus(bdd_ptr->mko_bc_ptr, bdd_ptr->mko_bus);
	mko_bc_transaction_start(bdd_ptr->mko_bc_ptr, MKO_BC_MODE_READ, bdd_ptr->mko_addr, BDD_MK_FRAME_SADDR_MEM_FRAME, (uint16_t*)&bdd_ptr->arch_frame, 32);
}

/**
  * @brief  запись измерения в fifo
	* @param  bdd_ptr указатель на структуру управления
	* @param  data указатель на массив с кадром данных
	* @retval  статус записи: 1 - ок, <0 - ошибка
  */
int8_t bdd_write_fifo(typeBDDStruct *bdd_ptr, typeBDDAcqValue* data)
{
	if (bdd_ptr->rec_num >= BDD_REC_FIFO_DEPTH){
		return -1;
	}
	else{
		memcpy((uint8_t*)&bdd_ptr->rec_fifo[bdd_ptr->rec_num], (uint8_t*)data, sizeof(typeBDDAcqValue));
		bdd_ptr->rec_num += 1;
		if (bdd_ptr->rec_num > bdd_ptr->rec_max) bdd_ptr->rec_max = bdd_ptr->rec_num;  //сбор информации о заполненности fifo
		return 1;
	}
}

/**
  * @brief  взятие измерения из fifo
	* @param  bdd_ptr указатель на структуру управления
	* @param  data указатель на массив с кадром данных
	* @retval  статус: 0 - fifo-пуст, 1 - ок
  */
int8_t bdd_read_fifo(typeBDDStruct* bdd_ptr, typeBDDAcqValue* data)
{
	if (bdd_ptr->rec_num == 0){
		return 0;
	}
	else{
		bdd_ptr->rec_num -= 1;
		memcpy((uint8_t*)data, (uint8_t*)&bdd_ptr->rec_fifo[0], sizeof(typeBDDAcqValue));
		memmove((uint8_t*)&bdd_ptr->rec_fifo[0], (uint8_t*)&bdd_ptr->rec_fifo[1], sizeof(typeBDDAcqValue)*bdd_ptr->rec_num);
		memset((uint8_t*)&bdd_ptr->rec_fifo[bdd_ptr->rec_num], 0x00, sizeof(typeBDDAcqValue)*(BDD_REC_FIFO_DEPTH - bdd_ptr->rec_num));
		return 1;
	}
}

/**
 * @brief переварачивание байт внутр 16-ти битных переменных
 * 
 * 
 * @param bdd_rec структура, принимаемая из модуля ДЭП по ВШ
 */
void _bdd_rec_rev(typeBDDAcqValue* bdd_rec)
{
  //
}

/**
  * @brief  инициализация циклограмм работы с ДЭП
	* @param  bdd_ptr указатель на структуру управления
  */
void bdd_meas_cycl_init(typeBDDStruct* bdd_ptr)
{
	uint8_t data[32] = {0};
	// циклограмма инициализации МПП
	cyclo_init(&bdd_ptr->meas_cyclo, "bdd_cyclo");
	//
	cyclo_add_step(&bdd_ptr->meas_cyclo, bdd_meas_cycl_read, (void*)bdd_ptr, 0, 100, data);
	cyclo_add_step(&bdd_ptr->meas_cyclo, bdd_meas_cycl_frame_forming, (void*)bdd_ptr, 0, 0, data);
}

/**
  * @brief  обертка функция для согласования типов
	* @param  ctrl_struct указатель на структуру управления
  */
int32_t bdd_meas_cycl_read(void* ctrl_struct, uint8_t* data)
{
	typeBDDStruct* bdd_ptr = (typeBDDStruct*) ctrl_struct;
	bdd_read_sys_frame(bdd_ptr);
	return 0;
}

/**
  * @brief  формирование кадра БДД
	* @param  ctrl_struct указатель на структуру управления
  */
int32_t bdd_meas_cycl_frame_forming(void* ctrl_struct, uint8_t* data)
{
	typeBDDStruct* bdd_ptr = (typeBDDStruct*) ctrl_struct;
	bdd_frame_forming(bdd_ptr);
	return 0;
}
