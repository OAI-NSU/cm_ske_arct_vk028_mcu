#ifndef _FRAMES_H_
#define _FRAMES_H_

#include "main.h"

#define FRAME_DEFINER_BASE_HUGE_SYSTEMS     0
#define FRAME_DEFINER_BASE_SMALL_SERIES     1

#pragma pack(push, 2)

/**
  * @brief  структура шаблона кадра кадра
  */
typedef struct // ситстемный кадр
{
  uint16_t label;  //+0 0
  uint16_t definer; //+2 1 
  uint16_t num; //+4 2
  uint32_t time; //+6 4
  //
  uint8_t data[52]; //+10
	//
  uint16_t crc16; //+62 30
}typeFrameStruct;

#pragma pack(pop)

// прототипы функций
uint16_t frame_definer(uint8_t frame_modification, uint16_t device_number,  uint16_t fabrication_num, uint8_t frame_type);
uint16_t frame_crc16(uint8_t *buf, uint8_t len);
uint8_t frame_get_type_from_definer(typeFrameStruct frame);
uint8_t frame_validate(typeFrameStruct frame);
// статические вспомогательные функции для составления кадров
uint32_t _rev_u32_by_u16(uint32_t var);

#endif
