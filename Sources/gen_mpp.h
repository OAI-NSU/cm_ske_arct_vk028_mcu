#ifndef _GEN_MPP_H_
#define _GEN_MPP_H_

#include <string.h>
#include <stdint.h>
#include "main.h"
#include "mpp.h"
#include "internal_bus.h"
#include "task_planner.h"
#include "cyclogramma.h"
#include "frames.h"
#include "clock.h"
#include "cm.h"

// дефайны для переменных
#define GMPP_REC_FIFO_DEPTH                   (4) 

#define GMPP_DEFAULT_INTERVAL_MS              (1000)

#define GMPP_EVENT_MEAS_INTERVAL_START        (1<<0)
#define GMPP_EVENT_MEAS_INTERVAL_DATA_READY   (1<<1)

//
#pragma pack(push, 2)

typedef struct // структура строки для матрицы МПП
{
  uint8_t amplitude[MPP_DEV_NUM];     // +0
  uint16_t time;                      // +6
}typeMPPMatrixString;                 // 8

typedef union // структура с кадром помеховой матрицы
{
  typeFrameStruct raw;
  struct{
    uint16_t label;               // +0 0
    uint16_t definer;             // +2 1
    uint16_t num;                 // +4 2
    uint32_t time;                // +6 3 4
    //
    typeMPPMatrixString matrix_string[MPP_DEV_NUM]; //+10-57 5-29
    //
    uint8_t reserved[4];          // +58
    //
    uint16_t crc16;               // +62
  } mpp;
}typeMPPMatrixFrameUnion;         // 64

typedef struct // структура данных одного канала для кадра окон МПП
{
     uint8_t bound[3];      //+0
     uint8_t num[3];        //+3
}typeMPPWindowFrameChannel;      //6

typedef union // структура с кадром помеховых окон
{
  typeFrameStruct raw;
  struct{
    uint16_t label;               //+0 0
    uint16_t definer;             //+2 1
    uint16_t num;                 //+4 2
    uint32_t time;                //+6 3-4
    //
    typeMPPWindowFrameChannel window_channel[6]; //+10-45 5-23
    //
    uint8_t reserved[16];         //+46
    //
    uint16_t crc16;               //+62 31
  } mpp;
}typeMPPWindowFrameUnion;         // 64

#pragma pack(pop)

/** 
  * @brief  структура управления МПП
  */
typedef struct
{
  //
  uint16_t self_num;          // номер устройства с точки зрения ЦМ
	const typeMPPStruct* mpp[MPP_DEV_NUM];
  //
  uint16_t device_number, win_frame_type, mtrx_frame_type;  //параметры прибора МПП, в котором он используется
  uint32_t *global_frame_num_ptr;
  //
  typeMPPMatrixString mpp_mtrx_str[MPP_DEV_NUM];
  typeMPPMatrixString mpp_cur_mtrx_str;
  //  
  typeMPPMatrixFrameUnion matrix_frame, mko_matrix_frame;
  typeMPPWindowFrameUnion win_frame, mko_win_frame;
  //
  uint8_t win_mpp_flag;
  uint8_t matrix_mpp_flag;
  uint32_t win_mpp_cnt;
  uint32_t matrix_mpp_cnt;
  //fifo
  typeFrameStruct frames_fifo[GMPP_REC_FIFO_DEPTH];
	uint8_t frames_fifo_num, frames_fifo_num_max;
	uint32_t fifo_error_cnt;
  //
	uint64_t last_call_time_us, call_interval_us;
	//
} typeGEN_MPP_Struct;

//
void gmpp_init( typeGEN_MPP_Struct* gmpp_ptr, 
                uint8_t self_num, 
                uint16_t device_number, 
                uint16_t mtrx_frame_type, 
                uint16_t win_frame_type, 
                uint32_t* gl_fr_num, 
                const typeMPPStruct* mpp_arr);
void gmpp_reset_parameters(typeGEN_MPP_Struct* gmpp_ptr);
//
int8_t gmpp_process_tp(void* ctrl_struct, uint64_t time_us, typeProcessInterfaceStruct* interface);
//
void gmpp_matrix_process_data(typeGEN_MPP_Struct* gmpp_ptr);
void gmpp_matrix_clear_data(typeGEN_MPP_Struct* gmpp_ptr);
void gmpp_matrix_frame_forming(typeGEN_MPP_Struct* gmpp_ptr);
void gmpp_matrix_mko_frame_forming(typeGEN_MPP_Struct* gmpp_ptr);
void gmpp_win_frame_forming(typeGEN_MPP_Struct* gmpp_ptr);
void gmpp_win_mko_frame_forming(typeGEN_MPP_Struct* gmpp_ptr);

//
void gmpp_fifo_init(typeGEN_MPP_Struct* gmpp_ptr);
int8_t gmpp_write_fifo(typeGEN_MPP_Struct* gmpp_ptr, typeFrameStruct* frame);
int8_t gmpp_read_fifo(typeGEN_MPP_Struct* gmpp_ptr, typeFrameStruct* frame);
//
#endif
