#ifndef _DEP_H_
#define _DEP_H_

#include <string.h>
#include "main.h"
#include "math.h"
#include "internal_bus.h"
#include "task_planner.h"
#include "cyclogramma.h"
#include "frames.h"
#include "clock.h"

// дефайны для переменных
#define DEP_DEFAULT_INTERVAL_MS (1000)

#define DEP_EVENT_MEAS_INTERVAL_START       (1<<0)
#define DEP_EVENT_MEAS_INTERVAL_DATA_READY  (1<<1)

#define DEP_REC_FIFO_DEPTH 8

// структуры данных
#pragma pack(push, 2)

/** 
  * @brief  структура для хранения и пересчета данных ДЭП
  */
typedef struct
{
  uint16_t Evalue_dep;
  uint8_t Dvalue_dep;
	uint8_t dep_ena;
}typeDEPProcess;

/** 
  * @brief  структура ДЭП для вставления в кадр кадра
  */
typedef struct
{
    uint16_t Evalue_dep1;     //+0
    uint8_t Dvalue_dep1;      //+2
    int8_t Temp_dep1;      //+3
    uint16_t Evalue_dep2;     //+4
    uint8_t Dvalue_dep2;      //+6
    int8_t Temp_dep2;      //+7
}typeDEPAcqValue;             //8

/** 
  * @brief  структура кадар ДЭП
  */
typedef union{
  typeFrameStruct raw;
  struct{
    uint16_t header[5];
    //
    typeDEPAcqValue meas[6]; //+10-57 5-28
    uint16_t change_int_num; //58 29
    uint16_t changed_meas_interv; //60 30
    //
    uint16_t crc16;
  } dep;
}typeDEPFrameUnion;

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
  typeDEPFrameUnion frame, mko_frame;
  uint8_t frame_data_ready;  // флаг готовности данных в памяти на отправку в другой процесс
  typeDEPProcess ch[2];
  typeDEPAcqValue last_meas;
  float last_field[2], last_freq[2];
  // fifo для обработки данных ДЭП
  typeDEPAcqValue rec_fifo[DEP_REC_FIFO_DEPTH];
  uint8_t rec_num, rec_max;
  uint16_t current_meas_interval;  // данные для хранения измерительного интервала
  uint16_t rec_num_to_meas_int_change;  // данные для хранения измерительного интервала
  //
  // general
	
  // cyclogram_ctrl
  typeCyclograma meas_cyclo;
} typeDEPStruct;

#pragma pack(pop)

//
void dep_init(typeDEPStruct* dep_ptr, uint8_t self_num, uint8_t id, uint16_t device_number, uint16_t frame_type, typeIBStruct* ib_ptr, uint32_t* gl_fr_num);
void dep_reset_parameters(typeDEPStruct* dep_ptr);
//
int8_t dep_process_tp(void* ctrl_struct, uint64_t time_us, typeProcessInterfaceStruct* interface);
int8_t dep_frame_forming(typeDEPStruct* dep_ptr);
void dep_mko_frame_forming(typeDEPStruct* dep_ptr);
//
void dep_constant_mode(typeDEPStruct* dep_ptr, uint32_t on_off);
void dep_start(typeDEPStruct *dep_ptr, uint16_t meas_num);
void dep_stop(typeDEPStruct *dep_ptr);
void dep_read_data(typeDEPStruct *dep_ptr);
float __dep_field_calculate(uint16_t raw_data);
float __dep_freq_calculate(uint8_t raw_data);

int8_t dep_write_fifo(typeDEPStruct *dep_ptr, typeDEPAcqValue* data);
int8_t dep_read_fifo(typeDEPStruct *dep_ptr, typeDEPAcqValue* data);

// функции для работы циклограмы измерительного интервала
void dep_meas_cycl_init(typeDEPStruct* dep_ptr);
int32_t dep_meas_cycl_struct_start(void* ctrl_struct, uint8_t* data);
int32_t dep_meas_cycl_struct_stop(void* ctrl_struct, uint8_t* data);
int32_t dep_meas_cycl_read_data(void* ctrl_struct, uint8_t* data);
int32_t dep_meas_cycl_frame_forming(void* ctrl_struct, uint8_t* data);
//

#endif
