#ifndef _FRAMES_H_
#define _FRAMES_H_

//*** Includes ***//
#include "main.h"
#include "crc16.h"
#include "clock.h"

//*** Defines ***//
// определитель кадров (метка для визуального или автоматического поиска кадров)
#define FRAME_MARK                    0x0FF1
// типы кадров 
#define SINGLE_FRAME_TYPE             0x00
#define ARCH_HEADER_FRAME_TYPE        0x01
#define ARCH_BODY_FRAME_TYPE          0x02


#define FRAME_FIFO_DEPTH              (8)
#define FRAME_FIFO_REC_LEN            (128)

//*** General parts of frames ***//
#pragma pack(push, 2)
/**
  * @brief  Union for easy acces to id_loc fields (2 bytes)
  */
typedef union{
  uint16_t full;
  struct {
    uint16_t data_code : 8;
    uint16_t flags : 4;
    uint16_t dev_id : 4;
  }fields;
} type_ID_Loc; //2

/**
  * @brief  single frame header type (12 bytes)
  */
typedef struct {
  uint16_t mark; //+0
  uint16_t id_sat; //+2
  type_ID_Loc id_loc; //+4
  uint16_t num; //+6
  uint32_t time; //+8
} type_SingleFrame_Header; //12

/**
  * @brief  head of archive header type (14 bytes)
  */
typedef struct {
  uint16_t mark; //+0
  uint16_t id_sat; //+2
  type_ID_Loc id_loc; //+4
  uint16_t num; //+6
  uint32_t time; //+8
  uint16_t arch_len; //+12
} type_ArchHeadFrame_Header; //14

/**
  * @brief  body of archive data type (10 bytes)
  */
typedef struct {
  uint16_t mark; //+0
  uint16_t id_sat; //+2
  type_ID_Loc id_loc; //+4
  uint16_t num; //+6
  uint16_t arch_num; //+8
} type_ArchBodyFrame_Header; //10

//Шаблоны кадров

/**
  * @brief  одиночный кадр
  */
typedef struct // одиночный кадр
{
  type_SingleFrame_Header header; //12
  //
  uint8_t data[114]; //+12
	//
  uint16_t crc16; //+126
}typeSingleFrameStruct;

/**
  * @brief  заголовок архивного кадра
  */
typedef struct
{
  type_ArchHeadFrame_Header header; //+0
  //
  uint8_t data[112]; //+14
	//
  uint16_t crc16;     //+126
}typeArchHeadFrameStruct;

/**
  * @brief  заголовок архивного кадра
  */
typedef struct
{
  type_ArchBodyFrame_Header header; //+0
  //
  uint8_t data[116]; //+10
	//
  uint16_t crc16; //+126
}typeArchBodyFrameStruct;

typedef struct{
  uint8_t array[FRAME_FIFO_DEPTH][FRAME_FIFO_REC_LEN];
  uint8_t rec_num, rec_max;
} typeFrameFifo;

#pragma pack(pop)

//*** Function prototypes ***//
uint8_t frame_create_header(uint8_t* header_ptr, uint16_t sat_id, uint8_t dev_id, uint8_t flag, uint8_t type, uint8_t d_code, uint16_t fr_num, uint16_t num);
void frame_crc16_calc(uint8_t* frame_ptr);
uint8_t frame_validate(uint8_t* frame_ptr);
uint16_t frame_get_definer(uint8_t* frame);
int16_t frame_create_archive(uint16_t sat_id, uint8_t dev_id, uint8_t flag, uint8_t d_code, uint16_t fr_num, uint8_t *frame_ptr, uint8_t* data, uint16_t data_len);
//
void frame_init_fifo(typeFrameFifo *fr_fifo_ptr);
int8_t frame_write_fifo(typeFrameFifo *fr_fifo_ptr, uint8_t* data);
int8_t frame_read_fifo(typeFrameFifo* fr_fifo_ptr, uint8_t* data);
#endif
