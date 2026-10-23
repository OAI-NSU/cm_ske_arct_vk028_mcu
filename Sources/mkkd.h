#ifndef _MKKD_H_k
#define _MKKD_H_

#include <string.h>
#include "main.h"
#include "math.h"
#include "internal_bus.h"
#include "mko_bc.h"
#include "task_planner.h"
#include "cyclogramma.h"
#include "frames.h"
#include "clock.h"


// дефайны для переменных
#define MKKD_DEFAULT_INTERVAL_MS (1000)

#define MKKD_EVENT_MEAS_INTERVAL_START       (1<<0)
#define MKKD_EVENT_MEAS_INTERVAL_DATA_READY  (1<<1)

#define MKKD_REC_FIFO_DEPTH 8


// структуры данных
#pragma pack(push, 2)

/** 
  * @brief  измерение МККД
  */
typedef struct
{
  uint16_t t_st_voltage;
  int16_t temp;
	uint16_t pwr_voltage;
	uint16_t ref_voltage;
}type_MKKD_Meas;

/** 
  * @brief  кадр с измерением МККД
  */
typedef union{
  typeFrameStruct raw;
  struct{
    uint16_t header[5];
    //
    type_MKKD_Meas meas[6]; //+10-57 5-28
    uint8_t reserve[4]; //+58
    //
    uint16_t crc16; //+62
  } dep;
}typeMKKD_Self_Frame;

/** 
  * @brief  структура управления ДЭП
  */
typedef struct
{
  // interfaces
  typeMKOBCStruct* mko_bc_ptr;
  // сfg
	uint16_t id;			          // id на внутренней шине или адрес МКО
	uint16_t self_num;          // номер устройства с точки зрения ЦМ
	uint16_t device_number, frame_type;  //параметры прибора, в котором он используется
  uint16_t interval_ms;
  uint16_t const_mode;
  uint32_t *global_frame_num_ptr;
  // to task_planner
  uint8_t meas_event_num;
  // data
  typeMKKD_Self_Frame frame[2];
  uint8_t frame_data_ready;  // флаг готовности данных в памяти на отправку в другой процесс
  // cyclogram_ctrl
  typeCyclograma meas_cyclo;
  // fifo для обработки данных MKKD
  typeFrameStruct fifo[MKKD_REC_FIFO_DEPTH];
  uint8_t rec_num, rec_max;
  //
  uint64_t last_call_time_us, call_interval_us;
} typeMKKDStruct;

#pragma pack(pop)

//
void mkkd_init(   typeMKKDStruct* mkkd_ptr, 
                  uint8_t self_num, 
                  uint8_t id, 
                  uint16_t device_number, 
                  uint16_t frame_type, 
                  typeMKOBCStruct* mko_bc_ptr, 
                  uint32_t* gl_fr_num);
void mkkd_reset_parameters(typeMKKDStruct* mkkd_ptr);
//
int8_t mkkd_process_tp(void* ctrl_struct, uint64_t time_us, typeProcessInterfaceStruct* interface);
int8_t mkkd_frame_forming(typeMKKDStruct* mkkd_ptr);
void mkkd_mko_frame_forming(typeMKKDStruct* mkkd_ptr);
//
void mkkd_constant_mode(typeMKKDStruct* mkkd_ptr, uint32_t on_off);
void mkkd_start(typeMKKDStruct *mkkd_ptr);
void mkkd_synch_time(typeMKKDStruct *mkkd_ptr);
void mkkd_read_data(typeMKKDStruct *mkkd_ptr);

int8_t mkkd_write_fifo(typeMKKDStruct *mkkd_ptr, typeFrameStruct* data);
int8_t mkkd_read_fifo(typeMKKDStruct* mkkd_ptr, typeFrameStruct* data);

// функции для работы циклограмы измерительного интервала
void mkkd_meas_cycl_init(typeMKKDStruct* mkkd_ptr);
int32_t mkkd_meas_cycl_start(void* ctrl_struct, uint8_t* data);
int32_t mkkd_meas_cycl_synch_time(void* ctrl_struct, uint8_t* data);
int32_t mkkd_meas_cycl_read_data(void* ctrl_struct, uint8_t* data);
int32_t mkkd_meas_cycl_frame_forming(void* ctrl_struct, uint8_t* data);
//

#endif
