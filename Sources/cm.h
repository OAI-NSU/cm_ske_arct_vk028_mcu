#ifndef _CM_H_
#define _CM_H_

#include "main.h"
#include <stdio.h>
#include <string.h>
#include "power_management.h"
#include "internal_bus.h"
#include "timers.h"
#include "frames.h"
#include "frame_mem.h"
#include "task_planner.h"
#include "mko_bc.h"
#include "stm.h"
#include "mpp_conf.h"
#include "pcu.h"

#define CM_HANDLER_INTERVAL_MS   500

/**
	* @brief  данная нумерация является общей для всей периферии и позволяет в общих списках понять смещение данного устройства 
	* @note  данный список также используется для нумерации источников/обработчиков событий от/к периферии (необходимо следить, что бы количество источников не превышало количество доступных событий)
*/
typedef enum devices_list
{
	CM, 		MPP1, 		MPP2, 		MPP3,
	MPP4, 		MPP5, 		MPP6, 		MPP7,
	MPP8, 		DEP_AD1, 	DEP_AD2, 	DEP_AD3,
	DEP_AD4, 	DEP_AD5, 	DEP_AD6, 	DEP_AD7,
	DEP_AD8, 	MKKD, 		GMPP, 
	DEV_NUM
} device_list;

//! Управление разрешением работы ускоренного режима для различных устройств
#define CM_DEVICE_MEAS_MODE_MASK 		0x05FFFE //! модули, работающие по измерительному интервалу
#define CM_DEVICE_SPEEDY_MODE_MASK 		0x05FFFE //! Ускоренный режим разрешен для всего, кроме ДИР (МККД) и ЦМ

enum cm_interval_list
{
	CM_INTERV_SYS, CM_INTERV_MEAS, CM_INTERVAL_SPEED, CM_INTERVAL_DBG,
	CM_1S_INTERVAL,
	CM_INTERV_NUMBER
};
// #define DEFAULT_CM_INTERV_VALUES_S					{10, 	120, 	2, 		10, 	1}
#define DEFAULT_CM_INTERV_VALUES_S					{5, 	5,	 	2, 		10, 	1}
#define DEFAULT_CM_DEFAULT_START_TIME_S				{15, 	15,		16, 	16, 	16}

#define CM_EVENT_MEAS_INTERVAL_START       			(1<<0)
#define CM_EVENT_MEAS_INTERVAL_DATA_READY  			(1<<1)
#define CM_EVENT_SYS_INTERVAL_START       			(1<<2)
#define CM_EVENT_SYS_INTERVAL_DATA_READY  			(1<<3)
#define CM_EVENT_SPEEDY_INTERVAL_START       		(1<<4)
#define CM_EVENT_SPEEDY_INTERVAL_DATA_READY  		(1<<5)

#define CM_FRAMES_FIFO_DEPTH 						16

#define CM_CFG_FRAME_TYPE							15  //TODO: проверит, почему данный тип оторван от остальных

//MKO sub_address that are used
/*
#define sys_frame_num 0x0F
#define mpp1_frame_num 0x01
#define mpp2_frame_num 0x02
#define mpp3_frame_num 0x03
#define mpp4_frame_num 0x04
#define mpp5_frame_num 0x05
#define mpp6_frame_num 0x06
#define matrix_frame 0x07
#define wind_frame 0x08
#define dep_frame_num 0x09
#define bdep_frame_num 0x0A
*/
#define CM_MKO_SA_SYS     							(1)

#define CM_MKO_SA_MPP1    							(2)
#define CM_MKO_SA_MPP2    							(3)
#define CM_MKO_SA_MPP3    							(4)
#define CM_MKO_SA_MPP4    							(5)

#define CM_MKO_SA_MATRIX  							(6)
#define CM_MKO_SA_WIND    							(7)

#define CM_MKO_SA_DEP_AD1 							(8)
#define CM_MKO_SA_DEP_AD2 							(9)
#define CM_MKO_SA_DEP_AD3 							(10)
#define CM_MKO_SA_DEP_AD4 							(11)
#define CM_MKO_SA_DEP_AD5 							(12)
#define CM_MKO_SA_DEP_AD6 							(13)
#define CM_MKO_SA_DEP_AD7 							(14)
#define CM_MKO_SA_DEP_AD8 							(15)

#define CM_MKO_SA_MKKD     							(16)

#define CM_MKO_SA_CMD								(0x11)

#define CM_MKO_SA_ARCH_REQUEST_CM					(0x12)
#define CM_MKO_SA_ARCH_REQUEST_MPP					(0x13)

#define CM_MKO_SA_ARCH_READ_CM						(0x14)  //TODO: Проверить нумерацию подадресов

#define CM_MKO_SA_TECH_CMD							(0x1E)
#define CM_MKO_SA_TECH_RD							(0x1C)

#define CM_MKO_SA_SYNC_TIME							(0x1D)

// debug interface that based on IB
#define CM_SELF_MB_ID  (1)  // для отладочных команд

// Half-set GPIO setting
#define HALF_SET_IO		(OUT_14)

/**
 * @brief  список команд МКО
 */
enum pcu_cmd_list
{
	CMD_TEST = 1,
	CMD_NUMBER
};


/**
 * @brief  список отладочных команд через ВШ
*/
enum cm_dbg_cmd_list
{
	CM_DBG_CMD_SWITCH_ON_OFF, CM_DBG_CMD_CM_RESET, CM_DBG_CMD_CM_CHECK_MEM, CM_DBG_CMD_CM_INIT,
	CM_DBG_CMD_ARCH_REQUEST,
	CM_DBG_CMD_NUMBER
};

// статусы ЦМ
#define CM_STATUS_WORK 					(1<<0)
#define CM_STATUS_CFG_HALF_SET	 		(1<<1)
#define CM_STATUS_FULL_MEM_READ	 		(1<<2)	//TODO: реализовать!
#define CM_STATUS_CFG_LOADED	 		(1<<3)
#define CM_STATUS_FULL			 		(0xFFFF)  //! переменная для полной установки/очистки статуса

//структуры кадров
#pragma pack(push, 1)
/** 
  * @brief  структура с данными для кадра ЦМ (!! длина обрежется по 52 байта)
  * @note  соседние байтовые поля в МКО меняются местами внутри одного слова
  */
typedef  struct
{
	// питание
	uint16_t currents[PWR_CH_NUMBER];   //+0
	uint16_t power_status;				//+26
	uint16_t power_state;				//+28
	//
	uint8_t nans_counter;  				//+30
	uint8_t nans_status;  				//+31
	uint8_t rst_cnter;  				//+32
	uint8_t cm_status;		  			//+33
	//
	uint32_t operation_time;  			//+34
	//
	int8_t stm_val;						//+38
	int8_t mcu_temp;					//+39
	//
	uint16_t read_ptr;					//+40
	uint16_t write_ptr; 				//+42
	//
	uint16_t sw_version;				//+44
	//
	uint8_t reserve[6]; 				//+46
}typeCMFrameReport;	//! 52 - общая длина

#pragma pack(pop)

#pragma pack(push, 2)

/**
 * @brief объединение для связки уровней кадров и полезных данных
 */
typedef union{
	typeFrameStruct raw;
	struct{
		uint16_t header[5];
		typeCMFrameReport body;
		uint16_t crc16;
	} sys;
}typeSysFrameUnion;

/** 
  * @brief  структура с конфигурацией ЦМ
  */
typedef  struct
{
	// питание
	uint16_t power_status;		//+0
	uint16_t power_state;		//+2
	//
	uint8_t rst_cnter;  		//+4
	uint8_t gup;				//+5
	//
	uint32_t operation_time;  	//+6
	//
	uint32_t write_ptr;			//+10
	uint32_t read_ptr;			//+14
	//
	uint8_t reserve[30];  		//+22
}typeCfgReport;      //52

typedef union{
	typeFrameStruct raw;
	struct{
		uint16_t header[5];
		typeCfgReport body;
		uint16_t crc16;
	} cfg;
}typeCfgFrameUnion;

/** 
  * @brief  структура с переменными управления ЦМ
  */
typedef  struct
{
	uint32_t speedy_mode_state;
	uint32_t speedy_mode_mask;
	uint64_t speedy_mode_timeout;
	//
	uint64_t intervals_us[CM_INTERV_NUMBER];
	uint64_t last_call_interval_times[CM_INTERV_NUMBER];
	uint64_t first_call_time[CM_INTERV_NUMBER];
	uint8_t first_call_status[CM_INTERV_NUMBER];
	//
	uint8_t meas_event, speedy_event;
	//
	uint32_t frame_end_to_end_number;
	//
	uint32_t sync_time_s;
	uint16_t rst_cnter, sync_num;
	int16_t diff_time;
	int8_t diff_time_fractional;
	//
	uint8_t status;  //! status работы ЦМ
	//
	uint32_t operation_time;
}typeCMControlStruct;

/**
  * @brief  общая структура программной модели ЦМ
  */
typedef struct
{
	//
	uint16_t id;			          	//! id на внутренней шине
	uint16_t self_num;          		//! номер устройства с точки зрения ЦМ
	uint16_t half_set_num;          	//! номер полукомплекта (актуально для приборов в с холодным резервированием)
	uint16_t device_number, frame_type; //! параметры прибора, в котором он используется
	//
	typeMKOBCStruct* mko_bc_ptr;
	type_GPIO_PCU* pcu_ptr;
	typeIBStruct *ib_ptr;
	typePower *pwr_ptr;
	typeFRAME_MEM mem;
	type_SINGLE_GPIO *hs_io_ptr;
	type_STM_Model *stm_ptr;
	//
	uint16_t sw_version;
	//
	typeSysFrameUnion frame;  //!системный кадр
	typeCfgFrameUnion current_cfg, loaded_cfg;  //! кадры с параметрами для сохранения
	uint8_t frame_data_ready;
	//
	typeFrameStruct frames_fifo[CM_FRAMES_FIFO_DEPTH];
	uint8_t frames_fifo_num, frames_fifo_num_max;
	uint32_t fifo_error_cnt;
	uint32_t global_frame_num;  //сквозной номер для формируемых кадров
	uint8_t const_mode;
	uint8_t ddii_pwr_on_flag;
	//
	typeCMControlStruct ctrl;
	//
	uint64_t last_call_time_us, call_interval_us;
	//
}typeCMModel;

#pragma pack(pop)
//
void cm_init(
				typeCMModel* cm_ptr, 
				uint8_t self_num, 
				uint8_t id, 
				typeMKOBCStruct *mko_bc_ptr,
				type_GPIO_PCU *pcu_ptr,
				typeIBStruct *ib_ptr, 
				typePower *pwr_ptr,
				type_SINGLE_GPIO *hs_io_ptr,
				type_STM_Model *stm_ptr,
				uint16_t device_number, 
				char* ver_str, 
				uint16_t frame_type
			);
void cm_reset_parameters(typeCMModel* cm_ptr);
int8_t cm_load_cfg(typeCMModel* cm_ptr);
void cm_save_cfg(typeCMModel* cm_ptr);
void cm_set_cfg(typeCMModel* cm_ptr);
void cm_get_cfg(typeCMModel* cm_ptr);
int8_t cm_process_tp(void* ctrl_struct, uint64_t time_us, typeProcessInterfaceStruct* interface);
int8_t cm_frame_receive(typeCMModel* cm_ptr, uint8_t* data);
void cm_frame_handling(typeCMModel* cm_ptr);
int8_t cm_write_fifo(typeCMModel* cm_ptr, typeFrameStruct* frame);
int8_t cm_read_fifo(typeCMModel* cm_ptr, typeFrameStruct* frame);
int8_t cm_interval_processor(typeCMModel* cm_ptr, uint8_t interval_id, uint64_t time_us);
// Управление настройками работы ЦМ
void cm_set_interval_value(typeCMModel* cm_ptr, uint16_t interval_number, uint16_t interval_value_s);
void cm_set_speedy_mode(typeCMModel* cm_ptr, uint16_t speedy_mask, uint16_t time_s);
//работа с системном кадром
void cm_frame_forming(typeCMModel* cm_ptr);
// Отладка через ВШ
__weak void cm_dbg_ib_command_handler(typeCMModel* cm_ptr);
// обработка командных сообщений ПКУ
__weak void cm_pcu_command_interface_handler(typeCMModel *cm_ptr);
void cm_constant_mode_ena(typeCMModel* cm_ptr, uint8_t mode);
uint8_t cm_set_clear_status(typeCMModel* cm_ptr, uint8_t status, uint8_t set_clear);
// Внутренние рабочие функции
void _buff_rev16(uint16_t *buff, uint8_t leng_16);
uint8_t uint16_to_log2_uint8_t(uint16_t var);
uint16_t get_val_from_bound(uint16_t val, uint16_t min, uint16_t max); //если число внутри границ - используется оно, если нет, то ближайшая граница
uint16_t check_val_in_bound(uint16_t val, uint16_t min, uint16_t max);
uint16_t get_version_from_str(char* var_str);

#endif
