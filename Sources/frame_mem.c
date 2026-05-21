/**
  ******************************************************************************
  * @file           : frame_mem.c
  * @version        : v1.0
  * @brief          : работа с общим пространством памяти, выделенным под архивные кадры + настройки
  * @author			    : Стюф Алексей/Alexe Styuf <a-styuf@yandex.ru>
  * @date			      : 2021.09.06
  ******************************************************************************
  */

#include "frame_mem.h"

/**
  * @brief  инициализация памяти для работы с архивом и параметрами
  * @param  mem_ptr указатель на программную модель памяти
  * @param  mode режим работы памяти
  * @retval статус инициализации
  */
int8_t fr_mem_init(typeFRAME_MEM* mem_ptr, uint8_t mode) 
{
  fram_init(&mem_ptr->fram);
  //
  mem_ptr->write_ptr = 0;
  mem_ptr->read_ptr = 0;
  mem_ptr->protected_point_ptr = 0;
  mem_ptr->protected_point_ena = 0;
  mem_ptr->check_result = 0;
  mem_ptr->check_time = 0;
  mem_ptr->mode = mode;
  //
  return 0;
}

/**
  * @brief  запись произвольного кадра в память
  * @param  mem_ptr указатель на программную модель памяти
  * @param  addr адрес кадра в общей памяти (единица - один кадр)
  * @param  frame указатель на кадр
  * @retval статус инициализации
  */
int8_t fr_mem_write_any_frame(typeFRAME_MEM* mem_ptr, uint32_t addr, uint8_t* frame)
{
  return fram_write_frame(&mem_ptr->fram, addr, frame);
}

/**
  * @brief  чтение произвольного кадра из памяти
  * @param  mem_ptr указатель на программную модель памяти
  * @param  addr адрес кадра в общей памяти (единица - один кадр)
  * @param  frame указатель на кадр
  * @retval статус инициализации
  */
int8_t fr_mem_read_any_frame(typeFRAME_MEM* mem_ptr, uint32_t addr, uint8_t* frame)
{
  return fram_read_frame(&mem_ptr->fram, addr, frame);
}

/**
  * @brief  запись кадра информации в архивную память по указателю записи
  * @param  mem_ptr указатель на программную модель памяти
  * @param  frame указатель на кадр
  * @retval 1 - кадр записан, 0 - кадр не записан
  */
int8_t fr_mem_write_data_frame(typeFRAME_MEM* mem_ptr, uint8_t* frame)
{
    uint32_t addr = 0;
    //управление указателем записи
    if (fr_mem_incr_wr_ptr(mem_ptr)){
      //запись кадра данных
      addr = _get_addr_from_ptr(mem_ptr->write_ptr);
      fr_mem_write_any_frame(mem_ptr, addr, frame);
      return 1;
    }
    //
    return 0;
}

/**
  * @brief  чтение кадра информации из архивной памяти  по указателю чтения
  * @param  mem_ptr указатель на программную модель памяти
  * @param  frame указатель на кадр
  * @retval 1 - прочтется новый кадр, 0 - прочтется старый кадр
  */
int8_t fr_mem_read_data_frame(typeFRAME_MEM* mem_ptr, uint8_t* frame)
{
    uint32_t addr = 0;
    int8_t status = 0;
    //управление указателем записи
    status = fr_mem_incr_rd_ptr(mem_ptr);
    //чтение кадра данных
    addr = _get_addr_from_ptr(mem_ptr->read_ptr);
    //
    fr_mem_read_any_frame(mem_ptr, addr, frame);
    return status;
}

/**
  * @brief  инкрементация указателя записи
  * @param  mem_ptr указатель на программную модель памяти
  * @retval 1 - кадр можно писать, 0 - кадр не должен быть записан
  */
int8_t fr_mem_incr_wr_ptr(typeFRAME_MEM* mem_ptr)
{
  uint32_t prot_area_ptr = 0;
  //
  prot_area_ptr = __fr_mem_calc_prot_area_ptr(mem_ptr);
  switch (mem_ptr->mode){
    case(FR_MEM_TYPE_WR_TO_RD_WITH_PROT_AREA):
      if ((mem_ptr->write_ptr >= prot_area_ptr) && (mem_ptr->write_ptr < mem_ptr->read_ptr)){
        return 0;
      }
      else{
        fr_mem_set_wr_ptr(mem_ptr, mem_ptr->write_ptr + 1);
        return 1;
      }
      // break;
    case(FR_MEM_TYPE_WR_TO_RD_WITH_PROT_POINT):
      if ((mem_ptr->write_ptr >= prot_area_ptr) && (mem_ptr->write_ptr < mem_ptr->protected_point_ptr) && (mem_ptr->protected_point_ena)){
        return 0;
      }
      else{
        fr_mem_set_wr_ptr(mem_ptr, mem_ptr->write_ptr + 1);
        return 1;
      }
      // break;
		}
	return 0;
}

/**
  * @brief  определение точки защищенной области
  * @param  mem_ptr указатель на программную модель памяти
  */
uint32_t __fr_mem_calc_prot_area_ptr(typeFRAME_MEM* mem_ptr)
{
  uint32_t prot_area_ptr = 0;
  switch (mem_ptr->mode){
    case(FR_MEM_TYPE_WR_TO_RD_WITH_PROT_AREA):
      if ((int32_t)(mem_ptr->read_ptr - FR_MEM_PROTECTED_AREA_FRAME_NUM) >= 0){
        prot_area_ptr = mem_ptr->read_ptr - FR_MEM_PROTECTED_AREA_FRAME_NUM;
      }
      else {
        prot_area_ptr = FRAM_TOTAL_VOLUME_FRAMES + (mem_ptr->read_ptr - FR_MEM_PROTECTED_AREA_FRAME_NUM);
      }
    break;
    case(FR_MEM_TYPE_WR_TO_RD_WITH_PROT_POINT):
      if (mem_ptr->protected_point_ena){
        if ((int32_t)(mem_ptr->protected_point_ptr - FR_MEM_PROTECTED_AREA_FRAME_NUM) >= 0){
          prot_area_ptr = mem_ptr->protected_point_ptr - FR_MEM_PROTECTED_AREA_FRAME_NUM;
        }
        else {
          prot_area_ptr = FRAM_TOTAL_VOLUME_FRAMES + (mem_ptr->protected_point_ptr - FR_MEM_PROTECTED_AREA_FRAME_NUM);
        }
      }
      else{
        prot_area_ptr = 0;
      }
    break;
  }
  return prot_area_ptr;
}

/**
  * @brief  инкрементация указателя чтения
  * @param  mem_ptr указатель на программную модель памяти
  * @retval 1 - прочтется новый кадр, 0 - прочтется старый кадр
  */
int8_t fr_mem_incr_rd_ptr(typeFRAME_MEM* mem_ptr)
{
  if ((mem_ptr->read_ptr == mem_ptr->write_ptr)){
    return 0;
  }
  else{
    fr_mem_set_rd_ptr(mem_ptr, mem_ptr->read_ptr + 1);
    return 1;
  }
}

/**
  * @brief  установка указателя чтения
  * @param  mem_ptr указатель на программную модель памяти
  * @param  ptr_val значение указателя для установки
  * @retval  0 - линейное увеличение указателя, 1 - сброс указателя на начало памяти
  */
int8_t fr_mem_set_rd_ptr(typeFRAME_MEM* mem_ptr, uint32_t ptr_val)
{
    mem_ptr->read_ptr = ptr_val;
    if (mem_ptr->read_ptr >= FR_MEM_WR_RD_PTR_MAX){
        mem_ptr->read_ptr = 0;
        return 1;
    }
    return 0;
}

/**
  * @brief  установка указателя записи
  * @param  mem_ptr указатель на программную модель памяти
  * @param  ptr_val значение указателя для установки
  * @retval  0 - линейное увеличение указателя, 1 - сброс указателя на начало памяти
  */
int8_t fr_mem_set_wr_ptr(typeFRAME_MEM* mem_ptr, uint32_t ptr_val)
{
    mem_ptr->write_ptr = ptr_val;
    if (mem_ptr->write_ptr >= FR_MEM_WR_RD_PTR_MAX){
        mem_ptr->write_ptr = 0;
        return 1;
    }
    return 0;
}

/**
 * @brief Установка защищенной точки
 * 
 * @param mem_ptr 
 */
void fr_mem_set_protected_point(typeFRAME_MEM* mem_ptr)
{
  if (mem_ptr->protected_point_ena == 0){
    mem_ptr->protected_point_ena = 1;
    mem_ptr->protected_point_ptr = mem_ptr->write_ptr;
    // printf("set_prot_point <%d>\n", mem_ptr->protected_point_ptr);
  }
}

/**
 * @brief Сброс защищенной точки
 * 
 * @param mem_ptr 
 */
void fr_mem_release_protected_point(typeFRAME_MEM* mem_ptr)
{
  mem_ptr->protected_point_ena = 0;
  mem_ptr->protected_point_ptr = 0;
  // printf("release_prot_point\n");
}

/**
  * @brief  установка указателя чтения в начало защищенной области
  * @param  mem_ptr указатель на программную модель памяти
  */
void fr_mem_set_rd_ptr_to_defense_area(typeFRAME_MEM* mem_ptr)
{
    fr_mem_set_rd_ptr(mem_ptr, __fr_mem_calc_prot_area_ptr(mem_ptr));
}


/**
  * @brief установка указателя чтения относительно указателя записи
  * Указать можно любое целочисленное значение
  * @param  mem_ptr указатель на программную модель памяти
  * @param  shift значение для сдвига, на которой указатель записи сдвинется относительно указателя чтения
  */
void fr_mem_set_rd_ptr_relative_to_wr_ptr(typeFRAME_MEM* mem_ptr, uint32_t shift)
{
  uint32_t ptr_val = 0;
  uint32_t real_shift = (shift % FR_MEM_WR_RD_PTR_MAX);
  printf("shift = %d\n", real_shift); 
  ptr_val = (real_shift >= mem_ptr->write_ptr) ? mem_ptr->write_ptr + (FR_MEM_WR_RD_PTR_MAX - real_shift) : mem_ptr->write_ptr - real_shift;
  printf("FR_MEM_WR_RD_PTR_MAX = %d; (mem_ptr->write_ptr - shift) = %d, ptr_val = %d\n", FR_MEM_WR_RD_PTR_MAX, (mem_ptr->write_ptr - shift), ptr_val); 
  fr_mem_set_rd_ptr(mem_ptr, ptr_val);
}

/**
  * @brief  неразрушающая проверка памяти
  * @param  mem_ptr указатель на программную модель памяти
  * @retval результат проверки:  0 - норма, не 0 - номер первого кадра с проблемой
  */
uint32_t fr_mem_check(typeFRAME_MEM* mem_ptr)
{
    uint32_t time = 0;
    uint32_t i=0;
    uint8_t test_frame[FRAM_FRAME_VOLUME_B] = {0}, read_frame[FRAM_FRAME_VOLUME_B] = {0}, mem_frame[FRAM_FRAME_VOLUME_B] = {0};
    //запоминаем время старта
    time = clock_get_time_s();
    //
    mem_ptr->check_result = 0;
    //создаем тестовые сигналы
    for (i=0; i<FRAM_FRAME_VOLUME_B; i++) {
        test_frame[i] = i & 0xFF;
    }
    for (i=0; i<FRAM_TOTAL_VOLUME_FRAMES; i++) {
        WDRST;
        //сохраняем данные из памяти, пишем тестовый кадр, читаем тестовый кадр, восстанавливаем данные
        fr_mem_read_any_frame(mem_ptr, i, mem_frame);
        fr_mem_write_any_frame(mem_ptr, i, test_frame);
        fr_mem_read_any_frame(mem_ptr, i, read_frame);
        fr_mem_write_any_frame(mem_ptr, i, mem_frame);
        //
        if(memcmp(test_frame, read_frame, FRAM_FRAME_VOLUME_B) != 0){
            mem_ptr->check_result = i;
        }
    }
	// подсчитываем время выполнения теста
	time = clock_get_time_s() - time;
  mem_ptr->check_time = time;
	return mem_ptr->check_result;
}

/**
  * @brief  форматирование памяти, а также сброс указателей
  * @details первые 4 байта в кадре - его номер в памяти, остальные заполнены 0xFEFE
  * @param  mem_ptr указатель на программную модель памяти
  */
void fr_mem_format(typeFRAME_MEM* mem_ptr)
{
    uint32_t time = 0;
    uint32_t addr=0;
    uint8_t fill_frame[FRAM_FRAME_VOLUME_B] = {0};
    //запоминаем время старта
    time = clock_get_time_s();
    //
    //создаем базовый филлер для памяти
    memset(fill_frame, 0xFE, FRAM_FRAME_VOLUME_B);
    //
    for (addr=0; addr<FRAM_VOLUME_FRAMES; addr++) {
        WDRST;
        *(uint32_t*)&fill_frame[0] = addr;
        //сохраняем данные из памяти, пишем тестовый кадр, читаем тестовый кадр, восстанавливаем данные
        fr_mem_write_any_frame(mem_ptr, addr, fill_frame);
        //
    }
    // подсчитываем время форматирование
    time = clock_get_time_s() - time;
    mem_ptr->format_time = time;
    // сброс указателей чтения и записи
    fr_mem_set_wr_ptr(mem_ptr, 0);
    fr_mem_set_rd_ptr(mem_ptr, 0);
}

/**
  * @brief  сохранение параметров в начало каждой из физических памятей
  * @param  mem_ptr указатель на программную модель памяти
  * @param  frame указатель на кадра размером 64 байта для записи
  */
void fr_mem_param_save(typeFRAME_MEM* mem_ptr, uint8_t* frame)
{
    uint8_t i=0;
    for (i=0; i<FRAM_USED_CHIP_NUMBER; i++){
        *(uint16_t*)&frame[FRAM_FRAME_VOLUME_B-2] = frame_crc16(frame, FRAM_FRAME_VOLUME_B-2);
        fr_mem_write_any_frame(mem_ptr, i*FRAM_VOLUME_FRAMES, frame);
    }
}

/**
  * @brief  чтение параметров из начала каждой из физических памятей с последующей проверкой по CRC
  * @param  mem_ptr указатель на программную модель памяти
  * @param  frame указатель на кадра размером 64 байта для полученных параметров
  * @retval  >= 0 - номер памяти из которой произошла загрузка параметров; <0 - загрузка параметров не удалась
  */
int8_t fr_mem_param_load(typeFRAME_MEM* mem_ptr, uint8_t* frame)
{
    uint8_t i;
    uint8_t read_frame[FRAM_FRAME_VOLUME_B] = {0};
    uint16_t crc16_val = 0, crc16_frame = 0;
    for (i=0; i<FRAM_USED_CHIP_NUMBER; i++)
    {
        fr_mem_read_any_frame(mem_ptr, i*FRAM_VOLUME_FRAMES, read_frame);
        crc16_val = frame_crc16(read_frame, 62);
        crc16_frame = *(uint16_t *)(&read_frame[FRAM_FRAME_VOLUME_B - 2]);
        if(crc16_val == crc16_frame)
        {
            memcpy(frame, read_frame, FRAM_FRAME_VOLUME_B);
            return i;
        }
    }
    return -1;
}

// static
/**
  * @brief  получение абсолютного адреса из указателя записи/чтения
  * @param  ptr указатель на кадр
  * @retval абсолютный адрес кадра в памяти
  */
uint32_t _get_addr_from_ptr(uint32_t ptr)
{
    uint32_t addr = 0, mem_num = 0;
    mem_num = (ptr / FRAM_VOLUME_FRAMES) + 1;
    addr = ptr + mem_num; 
    return addr;
}
