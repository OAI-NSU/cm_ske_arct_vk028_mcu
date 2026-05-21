  /**
  ******************************************************************************
  * @file           : gen_mpp.c
  * @version        : v1.0
  * @brief          : библиотека для управления отдельным каналом МПП, использует объект ВШ (internal_bus)
	* @note						: возможно использовать два отдельных канала в одном МПП как два независимых устройства (такой подход добавляет небольшой оверхэд, но объеденяет описание МПП с одним или несколькими каналами)
  * @author			    : Стюф Алексей/Alexe Styuf <a-styuf@yandex.ru>
  * @date						: 2021.09.22
  ******************************************************************************
  */

#include "gen_mpp.h"

/**
 * @brief инициализация структуры управления мпп
 * @param gmpp_ptr указатель на структуру управления мпп
 * @param self_num номер текущего устройства (индекс)
 * @param device_number уникальный номер устройства
 * @param mtrx_frame_type тип кадра для матрицы помех
 * @param win_frame_type тип кадра для окон помех
 * @param gl_fr_num указатель на глобальный счетчик кадров
 * @param mpp_arr массив структур параметров мпп
 */
void gmpp_init(	typeGEN_MPP_Struct* gmpp_ptr, 
				uint8_t self_num, 
				uint16_t device_number, 
				uint16_t mtrx_frame_type, 
				uint16_t win_frame_type, 
				uint32_t* gl_fr_num, 
				const typeMPPStruct* mpp_arr)
{
	gmpp_reset_parameters(gmpp_ptr);
	//
	gmpp_ptr->self_num = self_num;
	gmpp_ptr->device_number = device_number;
	gmpp_ptr->mtrx_frame_type = mtrx_frame_type;
	gmpp_ptr->win_frame_type = win_frame_type;
	gmpp_ptr->global_frame_num_ptr = gl_fr_num;
	gmpp_ptr->mpp[0] = &mpp_arr[0];
	gmpp_ptr->mpp[1] = &mpp_arr[1];
	gmpp_ptr->mpp[2] = &mpp_arr[2];
	gmpp_ptr->mpp[3] = &mpp_arr[3];
	gmpp_ptr->mpp[4] = &mpp_arr[4];
	gmpp_ptr->mpp[5] = &mpp_arr[5];
}

/**
	* @brief  сброс параметров структуры MPP
	* @param  mpp_ptr указатель на структуру управления
	*/
void gmpp_reset_parameters(typeGEN_MPP_Struct* gmpp_ptr)
{
	gmpp_ptr->last_call_time_us = 0;
	gmpp_ptr->call_interval_us = 0;
	//
	memset((uint8_t*)&gmpp_ptr->win_frame, 0x00, sizeof(typeFrameStruct));
	memset((uint8_t*)&gmpp_ptr->mko_win_frame, 0x00, sizeof(typeFrameStruct));
	memset((uint8_t*)&gmpp_ptr->matrix_frame, 0x00, sizeof(typeFrameStruct));
	memset((uint8_t*)&gmpp_ptr->mko_matrix_frame, 0x00, sizeof(typeFrameStruct));
	//
	gmpp_ptr->win_mpp_flag = 0;
	gmpp_ptr->matrix_mpp_flag = 0;
	gmpp_ptr->win_mpp_cnt = 0;
	gmpp_ptr->matrix_mpp_cnt = 0;
	//
	gmpp_fifo_init(gmpp_ptr);
}

/**
  * @brief  процесс обработки состояния МПП
	* @param  mpp_ptr указатель на структуру управления
	* @param  time_us время МК us
  */
int8_t gmpp_process_tp(void* ctrl_struct, uint64_t time_us, typeProcessInterfaceStruct* interface)
{
	uint8_t retval = 0;
	typeFrameStruct frame;
	typeGEN_MPP_Struct* gmpp_ptr = (typeGEN_MPP_Struct*)ctrl_struct;
	/// запус обработчика по интервалу
	if ((time_us - gmpp_ptr->last_call_time_us) > (GMPP_DEFAULT_INTERVAL_MS*1000)) {
		gmpp_ptr->last_call_time_us = time_us;
		// user code begin
		
		// сбор данных из МPP
		gmpp_matrix_process_data(gmpp_ptr);

		// user code end
		retval = 1;
	}
	//
	if (interface->event[gmpp_ptr->self_num] & GMPP_EVENT_MEAS_INTERVAL_START) 
	{
		interface->event[gmpp_ptr->self_num] &= ~GMPP_EVENT_MEAS_INTERVAL_START;
		retval = 1;
		//
		gmpp_matrix_frame_forming(gmpp_ptr);
		gmpp_matrix_mko_frame_forming(gmpp_ptr);
		gmpp_write_fifo(gmpp_ptr, &gmpp_ptr->mko_matrix_frame.raw);
		gmpp_matrix_clear_data(gmpp_ptr);
		//
		gmpp_win_frame_forming(gmpp_ptr);
		gmpp_win_mko_frame_forming(gmpp_ptr);		
		gmpp_write_fifo(gmpp_ptr, &gmpp_ptr->mko_win_frame.raw);
	}
	else {
		//
	}
	//обработка передачи данных
	if ((interface->event[gmpp_ptr->self_num] &= GMPP_EVENT_MEAS_INTERVAL_DATA_READY) == 0){
		if( gmpp_read_fifo(gmpp_ptr, &frame) == 1) {
			memcpy(interface->shared_mem, (uint8_t*)&frame, sizeof(typeFrameStruct));
			interface->event[gmpp_ptr->self_num] |= GMPP_EVENT_MEAS_INTERVAL_DATA_READY;
		}
	}
	return retval;
}

/**
 * @brief сбор и обработка данных для помеховой матрицы
 * 
 * @param gmpp_ptr 
 */
void gmpp_matrix_process_data(typeGEN_MPP_Struct* gmpp_ptr)
{	
	// выгрузка текущих данных
	gmpp_ptr->mpp_cur_mtrx_str.time = clock_get_time_s();
	for(uint8_t mpp_num=0; mpp_num<MPP_DEV_NUM; mpp_num++){
		if (gmpp_ptr->mpp[mpp_num]->matrix_raw.amplitude != 0xFEFE){
			gmpp_ptr->mpp_cur_mtrx_str.amplitude[mpp_num] = gmpp_ptr->mpp[mpp_num]->matrix_raw.amplitude;
		}
		else {
			gmpp_ptr->mpp_cur_mtrx_str.amplitude[mpp_num] = 0;
		}
	}
	// анализ текущих данных
	for(uint8_t mpp_num=0; mpp_num<MPP_DEV_NUM; mpp_num++){
		if (gmpp_ptr->mpp_cur_mtrx_str.amplitude[mpp_num] > (gmpp_ptr->mpp_mtrx_str[mpp_num].amplitude[mpp_num])){
			gmpp_ptr->mpp_mtrx_str[mpp_num].time = gmpp_ptr->mpp_cur_mtrx_str.time;
			memcpy(	(uint8_t*)&gmpp_ptr->mpp_mtrx_str[mpp_num].amplitude[0], 
					(uint8_t*)&gmpp_ptr->mpp_cur_mtrx_str.amplitude[0], 
					sizeof(gmpp_ptr->mpp_cur_mtrx_str.amplitude));
		}
	}
}

/**
 * @brief сброс статистики по помеховым матрицам
 * 
 * @param gmpp_ptr 
 */
void gmpp_matrix_clear_data(typeGEN_MPP_Struct* gmpp_ptr)
{
	memset(	(uint8_t*)&gmpp_ptr->mpp_mtrx_str[0], 0x00, sizeof(gmpp_ptr->mpp_mtrx_str));
	memset(	(uint8_t*)&gmpp_ptr->mpp_cur_mtrx_str, 0x00, sizeof(gmpp_ptr->mpp_cur_mtrx_str));
}

/**
 * @brief формирование кадра для данных помеховой матрицы
 * 
 * @param gmpp_ptr 
 */
void gmpp_matrix_frame_forming(typeGEN_MPP_Struct* gmpp_ptr)
{
	// формирование кадра
	gmpp_ptr->matrix_frame.raw.label = 0x0FF1;
	gmpp_ptr->matrix_frame.raw.definer = frame_definer(0, gmpp_ptr->device_number, NULL, gmpp_ptr->mtrx_frame_type);
	gmpp_ptr->matrix_frame.raw.num = ((*gmpp_ptr->global_frame_num_ptr)++)&0xFFFF;
	gmpp_ptr->matrix_frame.raw.time = (clock_get_time_s());
	//
	memset((uint8_t*)&gmpp_ptr->matrix_frame.raw.data, 0xFE, sizeof(gmpp_ptr->matrix_frame.raw.data));
	memcpy((uint8_t*)&gmpp_ptr->matrix_frame.mpp.matrix_string[0], (uint8_t*)&gmpp_ptr->mpp_mtrx_str[0], sizeof(gmpp_ptr->mpp_mtrx_str));
	//
	gmpp_ptr->matrix_frame.raw.crc16 = frame_crc16((uint8_t*)&gmpp_ptr->matrix_frame.raw, sizeof(typeFrameStruct) - 2);
}

/**
 * @brief формирование кадра для данных помеховой матрицы для перекладывания в МКО
 * 
 * @param gmpp_ptr 
 */
void gmpp_matrix_mko_frame_forming(typeGEN_MPP_Struct* gmpp_ptr)
{
	// формирование кадра
	gmpp_ptr->mko_matrix_frame.raw.label = gmpp_ptr->matrix_frame.raw.label;
	gmpp_ptr->mko_matrix_frame.raw.definer = gmpp_ptr->matrix_frame.raw.definer;
	gmpp_ptr->mko_matrix_frame.raw.num = gmpp_ptr->matrix_frame.raw.num;
	gmpp_ptr->mko_matrix_frame.raw.time = _rev_u32_by_u16(gmpp_ptr->matrix_frame.raw.time);
	//
	memcpy((uint8_t*)&gmpp_ptr->matrix_frame.mpp.matrix_string[0], (uint8_t*)&gmpp_ptr->mpp_mtrx_str[0], sizeof(gmpp_ptr->mpp_mtrx_str));
	for(uint8_t ch_num = 0; ch_num < 6; ch_num++) {
		gmpp_ptr->mko_matrix_frame.mpp.matrix_string[ch_num].amplitude[0] = gmpp_ptr->matrix_frame.mpp.matrix_string[ch_num].amplitude[1];
		gmpp_ptr->mko_matrix_frame.mpp.matrix_string[ch_num].amplitude[1] = gmpp_ptr->matrix_frame.mpp.matrix_string[ch_num].amplitude[0];
		gmpp_ptr->mko_matrix_frame.mpp.matrix_string[ch_num].amplitude[2] = gmpp_ptr->matrix_frame.mpp.matrix_string[ch_num].amplitude[3];
		gmpp_ptr->mko_matrix_frame.mpp.matrix_string[ch_num].amplitude[3] = gmpp_ptr->matrix_frame.mpp.matrix_string[ch_num].amplitude[2];
		gmpp_ptr->mko_matrix_frame.mpp.matrix_string[ch_num].amplitude[4] = gmpp_ptr->matrix_frame.mpp.matrix_string[ch_num].amplitude[5];
		gmpp_ptr->mko_matrix_frame.mpp.matrix_string[ch_num].amplitude[5] = gmpp_ptr->matrix_frame.mpp.matrix_string[ch_num].amplitude[4];

		gmpp_ptr->mko_matrix_frame.mpp.matrix_string[ch_num].time =gmpp_ptr->matrix_frame.mpp.matrix_string[ch_num].time;
	}
	//
	memcpy((uint8_t*)gmpp_ptr->mko_matrix_frame.mpp.reserved, (uint8_t*)gmpp_ptr->matrix_frame.mpp.reserved, sizeof(gmpp_ptr->mko_matrix_frame.mpp.reserved));
	//
	gmpp_ptr->mko_matrix_frame.raw.crc16 = frame_crc16((uint8_t*)&gmpp_ptr->mko_matrix_frame.raw, sizeof(typeFrameStruct) - 2);
}

/**
 * @brief формирование кадров помеховых окон
 * 
 * @param gmpp_ptr 
 */
void gmpp_win_frame_forming(typeGEN_MPP_Struct* gmpp_ptr)
{
	// формирование кадра
	gmpp_ptr->win_frame.raw.label = 0x0FF1;
	gmpp_ptr->win_frame.raw.definer = frame_definer(0, gmpp_ptr->device_number, NULL, gmpp_ptr->win_frame_type);
	gmpp_ptr->win_frame.raw.num = ((*gmpp_ptr->global_frame_num_ptr)++)&0xFFFF;
	gmpp_ptr->win_frame.raw.time = (clock_get_time_s());
	//
	for(uint8_t mpp_num=0; mpp_num<MPP_DEV_NUM; mpp_num++){
		for (uint8_t j = 0; j<3; j++) {
            gmpp_ptr->win_frame.mpp.window_channel[mpp_num].bound[j] = gmpp_ptr->mpp[mpp_num]->win_raw.bound[j] >> 4;
            gmpp_ptr->win_frame.mpp.window_channel[mpp_num].num[j] = gmpp_ptr->mpp[mpp_num]->win_raw.num[j];            
        }
	}
	memset((uint8_t*)gmpp_ptr->win_frame.mpp.reserved, 0xFE, sizeof(gmpp_ptr->win_frame.mpp.reserved));
	//
	gmpp_ptr->win_frame.raw.crc16 = frame_crc16((uint8_t*)&gmpp_ptr->win_frame.raw, sizeof(typeFrameStruct) - 2);
	
}

/**
 * @brief формирование кадров помеховых окон для перекладывания в МКО регистры
 * 
 * @param gmpp_ptr 
 */
void gmpp_win_mko_frame_forming(typeGEN_MPP_Struct* gmpp_ptr)
{
	// формирование кадра
	gmpp_ptr->mko_win_frame.raw.label = gmpp_ptr->win_frame.raw.label;
	gmpp_ptr->mko_win_frame.raw.definer = gmpp_ptr->win_frame.raw.definer;
	gmpp_ptr->mko_win_frame.raw.num = gmpp_ptr->win_frame.raw.num;
	gmpp_ptr->mko_win_frame.raw.time = _rev_u32_by_u16(gmpp_ptr->win_frame.raw.time);
	//
	for(uint8_t mpp_num=0; mpp_num<MPP_DEV_NUM; mpp_num++){
		gmpp_ptr->mko_win_frame.mpp.window_channel[mpp_num].bound[0] = gmpp_ptr->win_frame.mpp.window_channel[mpp_num].bound[1];
		gmpp_ptr->mko_win_frame.mpp.window_channel[mpp_num].bound[1] = gmpp_ptr->win_frame.mpp.window_channel[mpp_num].bound[0];
		gmpp_ptr->mko_win_frame.mpp.window_channel[mpp_num].bound[2] = gmpp_ptr->win_frame.mpp.window_channel[mpp_num].num[0];
		gmpp_ptr->mko_win_frame.mpp.window_channel[mpp_num].num[0] = gmpp_ptr->win_frame.mpp.window_channel[mpp_num].bound[2];            
		gmpp_ptr->mko_win_frame.mpp.window_channel[mpp_num].num[1] = gmpp_ptr->win_frame.mpp.window_channel[mpp_num].num[2];            
		gmpp_ptr->mko_win_frame.mpp.window_channel[mpp_num].num[2] = gmpp_ptr->win_frame.mpp.window_channel[mpp_num].num[1];            
	}
	memcpy((uint8_t*)gmpp_ptr->mko_win_frame.mpp.reserved, (uint8_t*)gmpp_ptr->win_frame.mpp.reserved, sizeof(gmpp_ptr->win_frame.mpp.reserved));
	//
	gmpp_ptr->mko_win_frame.raw.crc16 = frame_crc16((uint8_t*)&gmpp_ptr->mko_win_frame.raw, sizeof(typeFrameStruct) - 2);
}


/**
 * @brief инициализация fifo
 * 
 * @param gmpp_ptr 
 */
void gmpp_fifo_init(typeGEN_MPP_Struct* gmpp_ptr)
{
	// инициализация структуры управления кадрами
	gmpp_ptr->frames_fifo_num = 0x00;
	gmpp_ptr->frames_fifo_num_max = 0x00;
	gmpp_ptr->fifo_error_cnt = 0x00;
	memset((uint8_t*)gmpp_ptr->frames_fifo, 0x00, sizeof(gmpp_ptr->frames_fifo));
}

/**
 * @brief запись массива данных в fifo
 * 
 * @param gmpp_ptr 
 * @param frame 
 * @return int8_t -1 - fifo переполненно, 1 - запись произведена
 */
int8_t gmpp_write_fifo(typeGEN_MPP_Struct* gmpp_ptr, typeFrameStruct* frame)
{
	if (gmpp_ptr->frames_fifo_num >= GMPP_REC_FIFO_DEPTH){
		return -1;
	}
	else{
		memcpy((uint8_t*)&gmpp_ptr->frames_fifo[gmpp_ptr->frames_fifo_num], (uint8_t*)frame, sizeof(typeFrameStruct));
		gmpp_ptr->frames_fifo_num += 1;
		if (gmpp_ptr->frames_fifo_num > gmpp_ptr->frames_fifo_num_max) gmpp_ptr->frames_fifo_num_max = gmpp_ptr->frames_fifo_num;  //сбор информации о заполненности fifo
		return 1;
	}
}

/**
 * @brief чтение массива данных из fifo
 * 
 * @param gmpp_ptr 
 * @param frame 
 * @return int8_t 0 - fifo пуст, 1 - чтение произведено
 */
int8_t gmpp_read_fifo(typeGEN_MPP_Struct* gmpp_ptr, typeFrameStruct* frame)
{
	if (gmpp_ptr->frames_fifo_num == 0){
		return 0;
	}
	else{
		gmpp_ptr->frames_fifo_num -= 1;
		memcpy((uint8_t*)frame, (uint8_t*)&gmpp_ptr->frames_fifo[0], sizeof(typeFrameStruct));
		memmove((uint8_t*)&gmpp_ptr->frames_fifo[0], (uint8_t*)&gmpp_ptr->frames_fifo[1], sizeof(typeFrameStruct)*gmpp_ptr->frames_fifo_num);
		memset((uint8_t*)&gmpp_ptr->frames_fifo[gmpp_ptr->frames_fifo_num], 0x00, sizeof(typeFrameStruct)*(GMPP_REC_FIFO_DEPTH - gmpp_ptr->frames_fifo_num));
		return 1;
	}
}

