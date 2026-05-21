/**
  ******************************************************************************
  * @file           : frames.c
  * @version        : v1.0
  * @brief          : содержит структуры данных для формирования кадров по 128 байт
  * @author         : Стюф Алексей/Alexey Styuf <a-styuf@yandex.ru>
  ******************************************************************************
  */

#include "mka_frames.h"

/**
  * @brief  инициализация структуры кадра из 128 байт
  * @param  header_ptr указатель на структуру с заголовком
  * @param  sat_id уникальный номер аппарата
  * @param  dev_id номер устройства
  * @param  type тип кадра: 0-одиночный кадр, 1-заголовок архивного кадра, 2-тело архивного кадра
  * @param  d_code тип данных (согласно принятому соглашению: например "Маяк" или "ТМИ")
  * @param  num если тип кадра одиночный, то не используется, если архивный:
  *                   - одиночный: не проверяется
  *                   - заголовок архива: количество кадров
  *                   - тело архива: номер кадра архива (нулевой - это заголовок, соответственно первый кадр тела - 1й)
  * @retval длина получившегося заголовка (для разных типов данных длина разная), 0 - ошибка
  */
uint8_t frame_create_header(uint8_t* header_ptr, uint16_t sat_id, uint8_t dev_id, uint8_t flag, uint8_t type, uint8_t d_code, uint16_t fr_num, uint16_t num)
{
	type_SingleFrame_Header* s_header;
	type_ArchHeadFrame_Header* ah_header;
	type_ArchBodyFrame_Header* ab_header;

  switch(type){
    case SINGLE_FRAME_TYPE:
      s_header = (type_SingleFrame_Header*)header_ptr;
      s_header->mark = FRAME_MARK;
      s_header->id_sat = 0x8000 | sat_id;
      s_header->id_loc.fields.dev_id = dev_id & 0xF;
      s_header->id_loc.fields.flags = flag & 0x7;
      s_header->id_loc.fields.data_code = d_code & 0xFF;
      s_header->num = fr_num;
      s_header->time = clock_get_time_s();
      return sizeof(type_SingleFrame_Header);
    case ARCH_HEADER_FRAME_TYPE:
      ah_header = (type_ArchHeadFrame_Header*)header_ptr;
      ah_header->mark = FRAME_MARK;
      ah_header->id_sat = 0x8000 | sat_id;
      ah_header->id_loc.fields.dev_id = dev_id & 0xF;
      ah_header->id_loc.fields.flags = flag & 0x7;
      ah_header->id_loc.fields.data_code = d_code & 0xFF;
      ah_header->num = fr_num;
      ah_header->time = clock_get_time_s();
      ah_header->arch_len = num;
      return sizeof(type_ArchHeadFrame_Header);
    case ARCH_BODY_FRAME_TYPE:
      ab_header = (type_ArchBodyFrame_Header*)header_ptr;
      ab_header->mark = FRAME_MARK;
      ab_header->id_sat = 0x8000 | sat_id;
      ab_header->id_loc.fields.dev_id = dev_id & 0xF;
      ab_header->id_loc.fields.flags = (0x1 << 0) | (flag & 0x7);
      ab_header->id_loc.fields.data_code = d_code & 0xFF;
      ab_header->num = fr_num;
      ab_header->arch_num = num;
      return sizeof(type_ArchHeadFrame_Header);
  }
  return 0;
}

/**
  * @brief  подсчет и подстановка crc16 в готовый кадр из 128 байт
  * @param  frame_ptr: указатель на кадр
  */
void frame_crc16_calc(uint8_t* frame_ptr)
{
  uint16_t crc16;
  crc16 = norby_crc16_calc(frame_ptr, 126);
  frame_ptr[126] = (crc16 >> 8) & 0xFF;
  frame_ptr[127] = (crc16 >> 0) & 0xFF;
}

/**
  * @brief  проверка кадра на корректность
  * @param  frame указатель на начало кадра
  * @retval  >0 - кадр ок; 0 - некорректный кадр
  */
uint8_t frame_validate(uint8_t *frame)
{
  uint16_t label = *(uint16_t*)(frame);
  uint16_t crc16 = *(uint16_t*)(&frame[126]);
	if (label != 0x0FF1){  // проверка на метку кадра
		return 0;
	}
	else if(norby_crc16_calc((uint8_t*)frame, 126) != (__REVSH(crc16) & 0xFFFF)){ // проверка на контрольную сумму
		return 0;
	}
	return 1;
}


/**
  * @brief  запрос определителя кадра
  * @param  frame указатель на начало кадра
  * @retval  >0 - кадр ок; 0 - некорректный кадр
  */
uint16_t frame_get_definer(uint8_t *frame)
{
  uint16_t label = *(uint16_t*)(frame);
  uint16_t definer = *(uint16_t*)(&frame[4]);
  uint16_t crc16 = *(uint16_t*)(&frame[126]);
	if (label != 0x0FF1){  // проверка на метку кадра
		return 0;
	}
	else if(norby_crc16_calc((uint8_t*)frame, 126) != (__REVSH(crc16) & 0xFFFF)){ // проверка на контрольную сумму
		return 0;
	}
	return definer;
}

/**
 * @brief создание архива с данными, разбитого на кадры
 * 
 * @param sat_id 
 * @param dev_id 
 * @param flag 
 * @param d_code 
 * @param fr_num 
 * @param frame 
 * @param frame_num_ptr
 * @param data 
 * @param data_len 
 * @return int16_t 
 */
int16_t frame_create_archive(uint16_t sat_id, uint8_t dev_id, uint8_t flag, uint8_t d_code, uint16_t fr_num, uint8_t *frame_ptr, uint8_t* data, uint16_t data_len)
{
  typeArchHeadFrameStruct head;
  typeArchBodyFrameStruct body;
  uint16_t data_ptr = 0;
  uint16_t data_size = 0;
  uint8_t frame_num = 0;
  // title
  frame_create_header((uint8_t*)&head.header, sat_id, dev_id, flag, 1, d_code, fr_num, frame_num);
  data_size = (sizeof(head.data) > data_len) ? data_len : sizeof(head.data);
  memcpy((uint8_t*)head.data, (uint8_t*)data + data_ptr, data_size);
  if(data_size < sizeof(head.data)) memset((uint8_t*)head.data + data_size, 0xFE, sizeof(head.data) - data_size);
  data_ptr += data_size;
  frame_crc16_calc((uint8_t*)&head);
  memcpy((uint8_t*)frame_ptr, (uint8_t*)&head, sizeof(head));
  frame_num++;
  // body
  while(data_ptr < data_len){
    frame_create_header((uint8_t*)&body.header, sat_id, dev_id, flag, 2, d_code, fr_num, frame_num);
    data_size = (sizeof(body.data) > (data_len - data_ptr)) ? (data_len - data_ptr) : sizeof(body.data);
    memcpy((uint8_t*)body.data, (uint8_t*)data + data_ptr, data_size);
    if(data_size < sizeof(body.data)) memset((uint8_t*)body.data + data_size, 0xFE, sizeof(body.data) - data_size);
    data_ptr += data_size;
    frame_crc16_calc((uint8_t*)&body);
    memcpy((uint8_t*)frame_ptr + 128*frame_num, (uint8_t*)&body, sizeof(body));
    frame_num++;
  }
  return frame_num;
}

/**
  * @brief  запись измерения в fifo работаем с массивами по 128Б
	* @param  fr_fifo_ptr указатель на структуру управления
	* @param  data указатель на массив с кадром данных
	* @retval  статус записи: 1 - ок, <0 - ошибка
  */
void frame_init_fifo(typeFrameFifo *fr_fifo_ptr)
{
	memset((uint8_t*)fr_fifo_ptr, 0x00, sizeof(typeFrameFifo));
}

/**
  * @brief  запись измерения в fifo работаем с массивами по 128Б
	* @param  fr_fifo_ptr указатель на структуру управления
	* @param  data указатель на массив с кадром данных
	* @retval  статус записи: 1 - ок, <0 - ошибка
  */
int8_t frame_write_fifo(typeFrameFifo *fr_fifo_ptr, uint8_t* data)
{
	if (fr_fifo_ptr->rec_num >= FRAME_FIFO_DEPTH){
		return -1;
	}
	else{
		memcpy((uint8_t*)&fr_fifo_ptr->array[fr_fifo_ptr->rec_num][0], data, FRAME_FIFO_REC_LEN);
		fr_fifo_ptr->rec_num += 1;
		if (fr_fifo_ptr->rec_num > fr_fifo_ptr->rec_max) fr_fifo_ptr->rec_max = fr_fifo_ptr->rec_num;  //сбор информации о заполненности fifo
		return 1;
	}
}

/**
  * @brief  взятие измерения из fifo работаем с массивами по 128Б
	* @param  fr_fifo_ptr указатель на структуру управления
	* @param  data указатель на массив с кадром данных
	* @retval  статус: 0 - fifo-пуст, 1 - ок
  */
int8_t frame_read_fifo(typeFrameFifo *fr_fifo_ptr, uint8_t* data)
{
	if (fr_fifo_ptr->rec_num == 0){
		return 0;
	}
	else{
		fr_fifo_ptr->rec_num -= 1;
		memcpy((uint8_t*)data, &fr_fifo_ptr->array[0][0], FRAME_FIFO_REC_LEN);
		memmove(&fr_fifo_ptr->array[0][0], &fr_fifo_ptr->array[1][0], FRAME_FIFO_REC_LEN*fr_fifo_ptr->rec_num);
		memset((uint8_t*)&fr_fifo_ptr->array[fr_fifo_ptr->rec_num][0], 0x00, FRAME_FIFO_REC_LEN*(FRAME_FIFO_DEPTH - fr_fifo_ptr->rec_num));
		return 1;
	}
}
