#ifndef _DIR_H_
#define _DIR_H_

#include <string.h>
#include "main.h"
#include "internal_bus.h"
#include "task_planner.h"
#include "cyclogramma.h"
#include "frames.h"
#include "clock.h"


// дефайны для переменных
#define DIR_DEFAULT_INTERVAL_MS (10000)

#define DIR_EVENT_MEAS_INTERVAL_START       (1<<0)
#define DIR_EVENT_MEAS_INTERVAL_DATA_READY  (1<<1)

#define DIR_DEVICE_NUM    (5)
#define DIR_MEAS_NUM      (2)

#define DIR_REC_FIFO_DEPTH (DIR_MEAS_NUM*2)

#define DIR_MB_ADDR_RUN   (0x4000)
#define DIR_MB_ADDR_DATA  (0x4001)


// структуры данных
#pragma pack(push, 2)

/** 
  * @brief  структура одиночного измерения ДИР для одного канала
  */
typedef struct
{
    uint16_t dir;             //+0
    uint16_t temp;            //+2
}typeDIRSingleMeas;           //4

/** 
  * @brief  структура одиночного измерения ДИР для вставления в кадр
  */
typedef struct
{
    typeDIRSingleMeas ch[DIR_DEVICE_NUM];
}typeDIRMeas;                 //20

/** 
  * @brief  структура кадр ДИР
  */
typedef union{
  typeFrameStruct raw;
  struct{
    uint16_t header[5];         //+0
    //
    typeDIRMeas meas[DIR_MEAS_NUM];
    //
#if ((52-4*DIR_MEAS_NUM*DIR_DEVICE_NUM) == 0)
    //
#elif (((52-4*DIR_MEAS_NUM*DIR_DEVICE_NUM) > 0) && ((52-4*DIR_MEAS_NUM*DIR_DEVICE_NUM) < 52))
    uint8_t reserve[52-4*DIR_MEAS_NUM*DIR_DEVICE_NUM];
#else
    #warning "typeDIRFrameUnion reserve field size error."
#endif
    //
    uint16_t crc16;             //+60
  } dir;
}typeDIRFrameUnion;

/** 
  * @brief  структура управления ДЭП
  */
typedef struct
{
  // interfaces
  typeIBStruct* ib;
  // сfg
	uint16_t id;			          // id на внутренней шине
	uint16_t self_num;          // номер устройства с точки зрения ЦМ
	uint16_t device_number, frame_type;  //параметры прибора, в котором он используется
  uint16_t interval_ms;
  uint16_t const_mode;
  uint32_t *global_frame_num_ptr;
  // to task_planner
  uint8_t meas_event_num;
  uint64_t last_call_time_us;
  // data
  typeDIRFrameUnion frame;
  uint8_t frame_data_ready;  // флаг готовности данных в памяти на отправку в другой процесс
  //
  typeDIRMeas last_data;
  float voltage[DIR_DEVICE_NUM];
  float temp[DIR_DEVICE_NUM];
  // fifo для обработки данных ДЭП
  typeDIRMeas rec_fifo[DIR_REC_FIFO_DEPTH];
  uint8_t rec_num, rec_max;
  // general
	
  // cyclogram_ctrl
  typeCyclograma meas_cyclo;
  typeCyclograma speedy_cyclo;
} typeDIRStruct;

#pragma pack(pop)

//
void dir_init(typeDIRStruct* dir_ptr, uint8_t self_num, uint8_t id, uint16_t device_number, uint16_t frame_type, typeIBStruct* ib_ptr, uint32_t* gl_fr_num);
void dir_reset_parameters(typeDIRStruct* dir_ptr);
//
int8_t dir_process_tp(void* ctrl_struct, uint64_t time_us, typeProcessInterfaceStruct* interface);
int8_t dir_frame_forming(typeDIRStruct* dir_ptr);
//
void dir_constant_mode(typeDIRStruct* dir_ptr, uint32_t on_off);
void dir_read_data(typeDIRStruct *dir_ptr);

int8_t dir_write_fifo(typeDIRStruct *dir_ptr, typeDIRMeas* data);
int8_t dir_read_fifo(typeDIRStruct *dir_ptr, typeDIRMeas* data);

void _dir_rec_rev(typeDIRMeas* dir_rec);
// функции для работы циклограмы измерительного интервала
void dir_meas_cycl_init(typeDIRStruct* dir_ptr);
int32_t dir_meas_cycl_run(void* ctrl_struct, uint8_t* data);
int32_t dir_meas_cycl_read_data(void* ctrl_struct, uint8_t* data);
int32_t dir_meas_cycl_frame_forming(void* ctrl_struct, uint8_t* data);
//

#endif
