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
				uint32_t* gl_fr_num)
{
	gmpp_reset_parameters(gmpp_ptr);
	//
	gmpp_ptr->self_num = self_num;
	gmpp_ptr->device_number = device_number;
	gmpp_ptr->mtrx_frame_type = mtrx_frame_type;
	gmpp_ptr->win_frame_type = win_frame_type;
	gmpp_ptr->global_frame_num_ptr = gl_fr_num;
	//
	uint8_t id_array[GMPP_DEVICE_NUM] = { 	MPP1, MPP2, MPP3, MPP4, 
											MPP5, MPP6, MPP7, MPP8, 
											DEP_AD1, DEP_AD2, DEP_AD3, DEP_AD4,
											DEP_AD5, DEP_AD6, DEP_AD7, DEP_AD8 };
	memcpy(gmpp_ptr->device_list_id, id_array, sizeof(gmpp_ptr->device_list_id));
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
	gmpp_set_redirection_array(gmpp_ptr, GMPP_DEVICE_WORK_MASK_DEFAULT);
	gmpp_matrix_clear_rec(gmpp_ptr);
	gmpp_matrix_clear_data(gmpp_ptr);
	gmpp_win_clear_data(gmpp_ptr);
	//
	memset((uint8_t*)&gmpp_ptr->win_frame, 0x00, sizeof(typeFrameStruct) * GMPP_WIN_FRAME_NUM);
	memset((uint8_t*)&gmpp_ptr->mko_win_frame, 0x00, sizeof(typeFrameStruct) * GMPP_WIN_FRAME_NUM);
	memset((uint8_t*)&gmpp_ptr->matrix_frame, 0x00, sizeof(typeFrameStruct) * GMPP_MATRIX_FRAME_NUM);
	memset((uint8_t*)&gmpp_ptr->mko_matrix_frame, 0x00, sizeof(typeFrameStruct) * GMPP_MATRIX_FRAME_NUM);
	//
	gmpp_ptr->win_mpp_flag = 0;
	gmpp_ptr->matrix_mpp_flag = 0;
	gmpp_ptr->win_mpp_cnt = 0;
	gmpp_ptr->matrix_mpp_cnt = 0;
	//
	gmpp_ptr-> mpp_matrix_time_error = 0;
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
	uint8_t mpp_id =  0;
	/// запуск обработчика по интервалу
	if ((time_us - gmpp_ptr->last_call_time_us) > (GMPP_DEFAULT_INTERVAL_MS*1000)) {
		gmpp_ptr->last_call_time_us = time_us;
		// user code begin
		gmpp_matrix_process_data(gmpp_ptr);
		// user code end
		retval = 1;
	}
	//
	if (interface->event[gmpp_ptr->self_num] & GMPP_EVENT_MEAS_INTERVAL_START) 
	{
		interface->event[gmpp_ptr->self_num] &= ~GMPP_EVENT_MEAS_INTERVAL_START;
		retval = 1;
		// перекладываем кадры матрицы в fifo на отправку
		gmpp_matrix_frame_forming(gmpp_ptr);
		gmpp_matrix_mko_frame_forming(gmpp_ptr);
		for (uint8_t frame_num = 0; frame_num < GMPP_MATRIX_FRAME_NUM; frame_num++){
			gmpp_write_fifo(gmpp_ptr, &gmpp_ptr->mko_matrix_frame[frame_num].raw);
		}
		gmpp_matrix_clear_data(gmpp_ptr);
		// перекладываем кадры окон в fifo на отправку
		gmpp_win_frame_forming(gmpp_ptr);
		gmpp_win_mko_frame_forming(gmpp_ptr);		
		for (uint8_t frame_num = 0; frame_num < GMPP_WIN_FRAME_NUM; frame_num++){
			gmpp_write_fifo(gmpp_ptr, &gmpp_ptr->mko_win_frame[frame_num].raw);
		}
		gmpp_win_clear_data(gmpp_ptr);
	}
	else {
		//
	}
	// сбор данных матрицы
	for(uint8_t mpp_num = 0; mpp_num < GMPP_DEVICE_NUM; mpp_num++){
		mpp_id = gmpp_ptr->device_list_id[mpp_num];
		if (interface->event[mpp_id] & GMPP_EVENT_MATRIX_REC_DATA_READY){
			gmpp_ptr->matrix_rec[mpp_num] = *(typeGMPPMatrixVal*)&interface->shared_mem[mpp_id*128 + 66];
			interface->event[mpp_id] &= ~GMPP_EVENT_MATRIX_REC_DATA_READY;
		}
	}
	// сбор данных окон
	for(uint8_t mpp_num = 0; mpp_num < GMPP_DEVICE_NUM; mpp_num++){
		mpp_id = gmpp_ptr->device_list_id[mpp_num];
		if (interface->event[mpp_id] & GMPP_EVENT_WINDOW_REC_DATA_READY){
			gmpp_ptr->win_rec[mpp_num] = *(typeGMPPWindowChannel*)&interface->shared_mem[mpp_id*128 + 72];
			interface->event[mpp_id] &= ~GMPP_EVENT_WINDOW_REC_DATA_READY;
		}
	}
	//обработка передачи данных
	if ((interface->event[gmpp_ptr->self_num] & GMPP_EVENT_MEAS_INTERVAL_DATA_READY) == 0){
		if( gmpp_read_fifo(gmpp_ptr, &frame) == 1) {
			memcpy((uint8_t*)&interface->shared_mem[128 * GMPP], (uint8_t*)&frame, sizeof(typeFrameStruct));
			interface->event[gmpp_ptr->self_num] |= GMPP_EVENT_MEAS_INTERVAL_DATA_READY;
		}
	}
	return retval;
}

/**
 * @brief обработка данных для помеховой матрицы
 * 
 * @param gmpp_ptr 
 */
void gmpp_matrix_process_data(typeGEN_MPP_Struct* gmpp_ptr)
{	
	memset((uint8_t*)&gmpp_ptr->mpp_mtrx_str, 0x00, sizeof(typeMPPFullMatrixString));
	// выгрузка текущих данных
	for(uint8_t mpp_num=0; mpp_num<GMPP_DEVICE_NUM; mpp_num++){
		if (gmpp_ptr->matrix_rec[mpp_num].amplitude != 0xFEFE){
			gmpp_ptr->mpp_mtrx_str.amplitude[mpp_num] = (gmpp_ptr->matrix_rec[mpp_num].amplitude >> 4) & 0xFF;
			if(gmpp_ptr->mpp_mtrx_str.time != gmpp_ptr->matrix_rec[mpp_num].time){
				gmpp_ptr->mpp_mtrx_str.time = gmpp_ptr->matrix_rec[mpp_num].time;
				if(gmpp_ptr->mpp_mtrx_str.time != 0) gmpp_ptr->mpp_matrix_time_error++;
			}
		}
		else {
			gmpp_ptr->mpp_mtrx_str.amplitude[mpp_num] = 0;
		}
	}

	// анализ текущих данных
	for(uint8_t mpp_num=0; mpp_num<GMPP_DEVICE_NUM; mpp_num++){
		if (gmpp_ptr->mpp_mtrx_str.amplitude[mpp_num] > (gmpp_ptr->mpp_mtrx_array[mpp_num].amplitude[mpp_num])){
			gmpp_ptr->mpp_mtrx_array[mpp_num].time = gmpp_ptr->mpp_mtrx_str.time;
			memcpy(	(uint8_t*)&gmpp_ptr->mpp_mtrx_array[mpp_num].amplitude[0], 
					(uint8_t*)&gmpp_ptr->mpp_mtrx_str.amplitude[0], 
					sizeof(gmpp_ptr->mpp_mtrx_str.amplitude));
		}
	}

	gmpp_matrix_clear_rec(gmpp_ptr);
}

/**
 * @brief сброс собранных данных
 * 
 * @param gmpp_ptr 
 */
void gmpp_matrix_clear_rec(typeGEN_MPP_Struct* gmpp_ptr)
{
	memset((uint8_t*)&gmpp_ptr->matrix_rec[0], 0xFE, sizeof(typeGMPPMatrixVal) * GMPP_DEVICE_NUM);
	for (uint8_t num = 0; num < GMPP_DEVICE_NUM; num++){
		gmpp_ptr->matrix_rec[num].time = 0UL;
	}
}

/**
 * @brief сброс статистики по помеховым матрицам
 * 
 * @param gmpp_ptr 
 */
void gmpp_matrix_clear_data(typeGEN_MPP_Struct* gmpp_ptr)
{
	memset(	(uint8_t*)&gmpp_ptr->mpp_mtrx_array[0], 0x0, sizeof(gmpp_ptr->mpp_mtrx_array));
	memset(	(uint8_t*)&gmpp_ptr->mpp_mtrx_str, 0x00, sizeof(gmpp_ptr->mpp_mtrx_str));
}

/**
 * @brief сброс статистики по помеховым окнам
 * 
 * @param gmpp_ptr 
 */
void gmpp_win_clear_data(typeGEN_MPP_Struct* gmpp_ptr)
{
	memset((uint8_t*)&gmpp_ptr->win_rec[0], 0xFE, sizeof(typeGMPPWindowChannel) * GMPP_DEVICE_NUM);
}

/**
 * @brief установка массива индексов устройств по маске
 * @param gmpp_ptr указатель на структуру управления агрегатором МПП
 * @param redirection_mask бит N выбирает устройство с индексом N в device_list_id
 * @note Индексы идут по возрастанию; используются первые GMPP_USED_DEVICE_SIZE
 * устройств из младших GMPP_DEVICE_NUM битов. Свободные элементы равны 0xFF.
 * gmpp_used_device_flag содержит маску фактически выбранных устройств.
 */
void gmpp_set_redirection_array(typeGEN_MPP_Struct* gmpp_ptr, uint32_t redirection_mask)
{
	if (gmpp_ptr == NULL) return;

	memset(gmpp_ptr->redirection_array, 0xFF, sizeof(gmpp_ptr->redirection_array));
	gmpp_ptr->redirection_count = 0;
	gmpp_ptr->gmpp_used_device_flag = 0;

	for (uint8_t num = 0; num < GMPP_DEVICE_NUM && num < 32; num++){
		if ((redirection_mask & (1UL << num)) != 0){
			if (gmpp_ptr->redirection_count >= GMPP_USED_DEVICE_SIZE) break;
			gmpp_ptr->redirection_array[gmpp_ptr->redirection_count++] = num;
			gmpp_ptr->gmpp_used_device_flag |= (1UL << num);
		}
	}
}

/**
 * @brief получение индекса устройства по порядковому номеру в матрице или окнах
 * @param gmpp_ptr указатель на структуру управления агрегатором МПП
 * @param ordinal порядковый номер выбранного устройства, начиная с 0
 * @return Индекс устройства (0..GMPP_DEVICE_NUM-1) или 0xFF, если оно отсутствует.
 */
uint8_t gmpp_get_device_index(const typeGEN_MPP_Struct* gmpp_ptr, uint8_t ordinal)
{
	if (gmpp_ptr == NULL) return 0xFF;
	if ((ordinal >= GMPP_USED_DEVICE_SIZE) || (ordinal >= gmpp_ptr->redirection_count)) return 0xFF;

	uint8_t device_index = gmpp_ptr->redirection_array[ordinal];
	if (device_index >= GMPP_DEVICE_NUM) return 0xFF;

	return device_index;
}

/**
 * @brief формирование кадров накопленной помеховой матрицы
 * @note строки распределяются по кадрам последовательно, свободные байты заполняются 0xFE
 * @param gmpp_ptr указатель на структуру управления агрегатором МПП
 */
void gmpp_matrix_frame_forming(typeGEN_MPP_Struct* gmpp_ptr)
{
	uint32_t frame_time_s;
	uint8_t global_str_num = 0, current_device_num = 0, current_rec_num = 0;

	if (gmpp_ptr == NULL) return;
    if (gmpp_ptr->global_frame_num_ptr == NULL) return;

	frame_time_s = clock_get_time_s(); 

	for(uint8_t frame_num=0; frame_num<GMPP_MATRIX_FRAME_NUM; frame_num++){
		// формирование кадра
		gmpp_ptr->matrix_frame[frame_num].raw.label = 0x0FF1;
		gmpp_ptr->matrix_frame[frame_num].raw.definer = frame_definer(0, gmpp_ptr->device_number, NULL, gmpp_ptr->mtrx_frame_type+frame_num);
		gmpp_ptr->matrix_frame[frame_num].raw.num = ((*gmpp_ptr->global_frame_num_ptr)++)&0xFFFF;
		gmpp_ptr->matrix_frame[frame_num].raw.time = frame_time_s;
		//
		memset((uint8_t*)&gmpp_ptr->matrix_frame[frame_num].raw.data, 0xFE, sizeof(gmpp_ptr->matrix_frame[frame_num].raw.data));
		//
		for (uint8_t str_num=0; str_num < GMPP_MATRIX_LINE_NUM; str_num++){
			global_str_num = frame_num * GMPP_MATRIX_LINE_NUM + str_num;
			current_device_num = gmpp_get_device_index(gmpp_ptr, global_str_num);
			if (current_device_num != 0xFF){
				for (uint8_t num = 0; num < GMPP_USED_DEVICE_SIZE; num++){
					if ((current_rec_num = gmpp_get_device_index(gmpp_ptr, num)) != 0xFF){
						gmpp_ptr->matrix_frame[frame_num].mpp.matrix_string[str_num].amplitude[num] = \
										gmpp_ptr->mpp_mtrx_array[current_device_num].amplitude[current_rec_num];
					}
					else{
						gmpp_ptr->matrix_frame[frame_num].mpp.matrix_string[str_num].amplitude[num] = 0xFE;
					}
				}
				gmpp_ptr->matrix_frame[frame_num].mpp.matrix_string[str_num].time = \
									(frame_time_s - gmpp_ptr->mpp_mtrx_array[current_device_num].time) & 0xFFFF;
			}
			else{
				break;
			}
		}
		//
		gmpp_ptr->matrix_frame[frame_num].raw.crc16 = frame_crc16((uint8_t*)&gmpp_ptr->matrix_frame[frame_num].raw, sizeof(typeFrameStruct) - 2);
	}	
}

/**
 * @brief формирование кадра для данных помеховой матрицы для перекладывания в МКО
 * 
 * @param gmpp_ptr 
 */
void gmpp_matrix_mko_frame_forming(typeGEN_MPP_Struct* gmpp_ptr)
{
	for(uint8_t frame_num=0; frame_num<GMPP_MATRIX_FRAME_NUM; frame_num++){
		// формирование кадра
		gmpp_ptr->mko_matrix_frame[frame_num].raw.label = gmpp_ptr->matrix_frame[frame_num].raw.label;
		gmpp_ptr->mko_matrix_frame[frame_num].raw.definer = gmpp_ptr->matrix_frame[frame_num].raw.definer;
		gmpp_ptr->mko_matrix_frame[frame_num].raw.num = gmpp_ptr->matrix_frame[frame_num].raw.num;
		gmpp_ptr->mko_matrix_frame[frame_num].raw.time = _rev_u32_by_u16(gmpp_ptr->matrix_frame[frame_num].raw.time);
		//
		for(uint8_t num = 0; num < GMPP_MATRIX_LINE_NUM; num++) {
			for(uint8_t ch_num = 0; ch_num < GMPP_USED_DEVICE_SIZE; ch_num++) {
				gmpp_ptr->mko_matrix_frame[frame_num].mpp.matrix_string[num].amplitude[ch_num] = gmpp_ptr->matrix_frame[frame_num].mpp.matrix_string[num].amplitude[ch_num];
			}
			gmpp_ptr->mko_matrix_frame[frame_num].mpp.matrix_string[num].time = gmpp_ptr->matrix_frame[frame_num].mpp.matrix_string[num].time;
		}
		//
		memcpy((uint8_t*)gmpp_ptr->mko_matrix_frame[frame_num].mpp.reserved, (uint8_t*)gmpp_ptr->matrix_frame[frame_num].mpp.reserved, sizeof(gmpp_ptr->mko_matrix_frame[frame_num].mpp.reserved));
		//
		gmpp_ptr->mko_matrix_frame[frame_num].raw.crc16 = frame_crc16((uint8_t*)&gmpp_ptr->mko_matrix_frame[frame_num].raw, sizeof(typeFrameStruct) - 2);
	}
}

/**
 * @brief формирование кадров помеховых окон
 * 
 * @param gmpp_ptr 
 */
void gmpp_win_frame_forming(typeGEN_MPP_Struct* gmpp_ptr)
{
	uint32_t frame_time_s;
	uint8_t global_str_num = 0, current_device_num = 0;

	if (gmpp_ptr == NULL) return;
    if (gmpp_ptr->global_frame_num_ptr == NULL) return;

	frame_time_s = clock_get_time_s(); 

	for(uint8_t frame_num=0; frame_num<GMPP_WIN_FRAME_NUM; frame_num++){
		// формирование кадра
		gmpp_ptr->win_frame[frame_num].raw.label = 0x0FF1;
		gmpp_ptr->win_frame[frame_num].raw.definer = frame_definer(0, gmpp_ptr->device_number, NULL, gmpp_ptr->win_frame_type+frame_num);
		gmpp_ptr->win_frame[frame_num].raw.num = ((*gmpp_ptr->global_frame_num_ptr)++)&0xFFFF;
		gmpp_ptr->win_frame[frame_num].raw.time = frame_time_s;
		//
		memset((uint8_t*)&gmpp_ptr->win_frame[frame_num].raw.data, 0xFE, sizeof(gmpp_ptr->win_frame[frame_num].raw.data));
		//
		for (uint8_t str_num=0; str_num < GMPP_WIN_REC_NUM; str_num++){
			global_str_num = frame_num * GMPP_WIN_REC_NUM + str_num;
			current_device_num = gmpp_get_device_index(gmpp_ptr, global_str_num);
			if (current_device_num != 0xFF){
				memcpy(	(uint8_t*)&gmpp_ptr->win_frame[frame_num].mpp.window_channel[str_num], 
						(uint8_t*)&gmpp_ptr->win_rec[current_device_num], 
						sizeof(typeGMPPWindowChannel));
			}
			else{
				memset(	(uint8_t*)&gmpp_ptr->win_frame[frame_num].mpp.window_channel[str_num], 
						0xFE, 
						sizeof(typeGMPPWindowChannel));
			}
		}
		//
		gmpp_ptr->win_frame[frame_num].raw.crc16 = frame_crc16((uint8_t*)&gmpp_ptr->win_frame[frame_num].raw, sizeof(typeFrameStruct) - 2);
	}
	
}

/**
 * @brief формирование кадров помеховых окон для перекладывания в МКО регистры
 * 
 * @param gmpp_ptr 
 */
void gmpp_win_mko_frame_forming(typeGEN_MPP_Struct* gmpp_ptr)
{
	for(uint8_t frame_num=0; frame_num<GMPP_WIN_FRAME_NUM; frame_num++){
		// формирование кадра
		gmpp_ptr->mko_win_frame[frame_num].raw.label = gmpp_ptr->win_frame[frame_num].raw.label;
		gmpp_ptr->mko_win_frame[frame_num].raw.definer = gmpp_ptr->win_frame[frame_num].raw.definer;
		gmpp_ptr->mko_win_frame[frame_num].raw.num = gmpp_ptr->win_frame[frame_num].raw.num;
		gmpp_ptr->mko_win_frame[frame_num].raw.time = _rev_u32_by_u16(gmpp_ptr->win_frame[frame_num].raw.time);
		//
		for(uint8_t num = 0; num < GMPP_WIN_REC_NUM; num++) {
			gmpp_ptr->mko_win_frame[frame_num].mpp.window_channel[num] = gmpp_ptr->win_frame[frame_num].mpp.window_channel[num];
		}
		memcpy(	(uint8_t*)gmpp_ptr->mko_win_frame[frame_num].mpp.reserved, 
				(uint8_t*)gmpp_ptr->win_frame[frame_num].mpp.reserved, 
				sizeof(gmpp_ptr->win_frame[frame_num].mpp.reserved));
		//
		gmpp_ptr->mko_win_frame[frame_num].raw.crc16 = frame_crc16((uint8_t*)&gmpp_ptr->mko_win_frame[frame_num].raw, sizeof(typeFrameStruct) - 2);
	}
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

