#ifndef _FRAME_MEM_H_
#define _FRAME_MEM_H_

#include <stdio.h>
#include "main.h"
#include "frames.h"
#include "spi_fram.h"
#include "clock.h"

#define FR_MEM_TYPE_WR_TO_RD_WITH_PROT_AREA 0  //! режим работы, при котором указатель чтения инкриминируется, указатель записи инкриминируется, но при достижении (указателя чтения - FR_MEM_PROTECTED_AREA_FRAME_NUM) останавливается запись
#define FR_MEM_TYPE_WR_TO_RD_WITH_PROT_POINT 1  //! режим работы, при котором указатель чтения инкриминируется, указатель записи инкриминируется, но при достижении (Защищенной точки - FR_MEM_PROTECTED_AREA_FRAME_NUM) останавливается запись. Защищенная точка устанавливается вручную.

#define FRAM_TOTAL_VOLUME_B (FRAM_VOLUME_B*FRAM_USED_CHIP_NUMBER)
#define FRAM_TOTAL_VOLUME_FRAMES (FRAM_TOTAL_VOLUME_B/64)

#define FR_MEM_WR_RD_PTR_MAX (FRAM_TOTAL_VOLUME_FRAMES - FRAM_USED_CHIP_NUMBER)

#define FR_MEM_PROTECTED_AREA_FRAME_NUM (FR_MEM_WR_RD_PTR_MAX/10)   //TODO: уточнить необходимое количество кадров для защищенной области


// Важно: первый кадр каждой памяти отведен под хранение данных ЦМ. 
typedef struct {
  type_FRAM fram;
  uint8_t mode;  //todo: заготовка под управление типом чтения через указатель чтения
  uint32_t write_ptr, read_ptr, protected_point_ptr;
  uint8_t protected_point_ena; //включение защиты по принципу защищенной точки
  uint32_t check_result, check_time, format_time;
}typeFRAME_MEM;

//
int8_t fr_mem_init(typeFRAME_MEM* mem_ptr, uint8_t mode);
int8_t fr_mem_write_any_frame(typeFRAME_MEM* mem_ptr, uint32_t addr, uint8_t* frame);
int8_t fr_mem_read_any_frame(typeFRAME_MEM* mem_ptr, uint32_t addr, uint8_t* frame);
int8_t fr_mem_write_data_frame(typeFRAME_MEM* mem_ptr, uint8_t* frame);
int8_t fr_mem_read_data_frame(typeFRAME_MEM* mem_ptr, uint8_t* frame);
int8_t fr_mem_incr_wr_ptr(typeFRAME_MEM* mem_ptr);
uint32_t __fr_mem_calc_prot_area_ptr(typeFRAME_MEM* mem_ptr);
int8_t fr_mem_incr_rd_ptr(typeFRAME_MEM* mem_ptr);
int8_t fr_mem_set_wr_ptr(typeFRAME_MEM* mem_ptr, uint32_t ptr_val);
int8_t fr_mem_set_rd_ptr(typeFRAME_MEM* mem_ptr, uint32_t ptr_val);
void fr_mem_set_protected_point(typeFRAME_MEM* mem_ptr);
void fr_mem_release_protected_point(typeFRAME_MEM* mem_ptr);
void fr_mem_set_rd_ptr_to_defense_area(typeFRAME_MEM* mem_ptr);
void fr_mem_set_rd_ptr_relative_to_wr_ptr(typeFRAME_MEM* mem_ptr, uint32_t shift);
uint32_t fr_mem_check(typeFRAME_MEM* mem_ptr);
void fr_mem_format(typeFRAME_MEM* mem_ptr);
void fr_mem_param_save(typeFRAME_MEM* mem_ptr, uint8_t* frame);
int8_t fr_mem_param_load(typeFRAME_MEM* mem_ptr, uint8_t* frame);
// static
uint32_t _get_addr_from_ptr(uint32_t ptr);

#endif
