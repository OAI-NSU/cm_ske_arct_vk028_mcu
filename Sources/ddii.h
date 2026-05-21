#ifndef _DDII_H_
#define _DDII_H_

#include <string.h>
#include "main.h"
#include "task_planner.h"
#include "cyclogramma.h"
#include "frames.h"
#include "mko_bc.h"
#include "clock.h"


// дефайны для переменных
#define DDII_DEFAULT_INTERVAL_MS (10000)

#define DDII_EVENT_MEAS_INTERVAL_START       (1<<0)
#define DDII_EVENT_MEAS_INTERVAL_DATA_READY  (1<<1)

#define DDII_MEAS_NUMBER 1
#define DDII_REC_FIFO_DEPTH 2

#define DDII_MKO_SA_SYS_FRAME     1
#define DDII_MKO_SA_MEAS_FRAME    2
#define DDII_MKO_SA_CMD           17

/**
  * @brief  список команд МКО
*/
enum ddii_cmd_list
{
	CMD_DDII_SYNCH_TIME, CMD_DDII_INIT, CMD_DDII_SET_INTERVAL, CMD_DDII_SET_MPP_OFFSET,
	CMD_DDII_CONST_MODE, CMD_DDII_CURRENT_LVL, CMD_DDII_PWR_CH_CTRL, CMD_DDII_START,
	CMD_DDII_NUMBER
};

//
#pragma pack(push, 2)

/**
 * @brief программная отчета о работе модуля высоковольтного ВИП
 * 
 */
typedef struct
{
  uint16_t h_voltage;
  uint16_t current;
  uint8_t state;
  uint8_t reserve;
}type_DDII_HVIP_frame_report;

/** 
  * @brief  структура с данными для кадра ЦМ
  */
typedef  struct
{
	uint8_t temp[4];		//! +0
	type_DDII_HVIP_frame_report hvip_report[4];
	uint8_t reserve[24];  	//! +4
	//
}typeDDIISysFrameReport;	//! 52 - общая длина

/**
 * @brief объединение для свзяки уровней кадров и полезных данных
 */
typedef union{
	typeFrameStruct raw;
	struct{
		uint16_t header[5];
		typeDDIISysFrameReport body;
		uint16_t crc16;
	} sys;
}typeDDIISysFrameUnion;

/** 
  * @brief  структура управления МПП
  */
typedef struct
{
  // interfaces
  typeMKOBCStruct* mko_bc_ptr;
  // сfg
	uint16_t mko_addr;			          // id на внутренней шине
	uint16_t mko_bus;			          // id на внутренней шине
	uint16_t self_num;          // номер устройства с точки зрения ЦМ
	uint16_t device_number, frame_type;  //параметры прибора МПП, в котором он используется
  uint16_t interval_ms;
  uint16_t const_mode;
  uint32_t *global_frame_num_ptr;
  // to task_planner
  uint8_t meas_event_num;
  uint64_t last_call_time_us;
  // data
  typeDDIISysFrameUnion sys_ddii_frame;
  uint8_t frame_data_ready;  // флаг готовности данных в памяти на отправку в другой процесс
  // fifo для обработки данных ДДИИ
  typeDDIISysFrameUnion rec_fifo[DDII_REC_FIFO_DEPTH];
  uint8_t rec_num, rec_max;
  // general
	typeDDIISysFrameUnion sys_raw_frame;
  // cyclogram_ctrl
  typeCyclograma meas_cyclo;
} typeDDIIStruct;

#pragma pack(pop)

//
void ddii_init(typeDDIIStruct* ddii_ptr, uint8_t self_num, uint8_t mko_addr, uint16_t device_number, uint16_t frame_type, typeMKOBCStruct* mko_bc_ptr, uint8_t mko_bus, uint32_t* gl_fr_num);
void ddii_reset_parameters(typeDDIIStruct* ddii_ptr);
//
int8_t ddii_process_tp(void* ctrl_struct, uint64_t time_us, typeProcessInterfaceStruct* interface);
int8_t ddii_frame_forming(typeDDIIStruct* ddii_ptr);
//
void ddii_meas_request(typeDDIIStruct* ddii_ptr);
void ddii_read_data_frame(typeDDIIStruct *ddii_ptr);
void ddii_const_mode(typeDDIIStruct *ddii_ptr, uint8_t mode);
//
int8_t ddii_write_fifo(typeDDIIStruct *ddii_ptr, typeDDIISysFrameUnion* data);
int8_t ddii_read_fifo(typeDDIIStruct* ddii_ptr, typeDDIISysFrameUnion* data);
// функции для работы циклограмы измерительного интервала
void ddii_meas_cycl_init(typeDDIIStruct* ddii_ptr);
int32_t ddii_meas_cycl_request(void* ctrl_struct, uint8_t* data);
int32_t ddii_meas_cycl_read(void* ctrl_struct, uint8_t* data);
int32_t ddii_meas_cycl_frame_forming(void* ctrl_struct, uint8_t* data);
//

#endif
