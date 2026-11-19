#ifndef _DEP_H_
#define _DEP_H_

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
#define DEP_DEFAULT_INTERVAL_MS (1000)

#define DEP_EVENT_MEAS_INTERVAL_START       (1 << 0)
#define DEP_EVENT_MEAS_INTERVAL_DATA_READY  (1 << 1)
#define DEP_EVENT_MATRIX_REC_DATA_READY     (1 << 7)
#define DEP_EVENT_WINDOW_REC_DATA_READY     (1 << 8)

#define DEP_REC_FIFO_DEPTH 8
#define DEP_FRAME_FIFO_DEPTH 8

// значения по умолчанию для границ помеховых окон
#define DEP_RP_WIN_BND_DEFAULT_1 0x01
#define DEP_RP_WIN_BND_DEFAULT_2 0x08
#define DEP_RP_WIN_BND_DEFAULT_3 0x20

//номер режимов ДЭП
enum dep_mode_enum
{
	DEP_MODE_IB, DEP_MODE_MKO,
  DEP_MODE_NUM
};

// структуры данных
#pragma pack(push, 1)


/**
 * @brief значение оконных параметров канала МПП
 *
 */
typedef struct
{
  uint16_t bound[3];      // +0  // границы трёх окон
  uint16_t num[3];        // +6  // счётчики трёх окон
} typeDEPADRPWindowChannel;  // 12


/**
 * @brief исходное измерение канала для формирования помеховой матрицы
 */
typedef struct
{
  uint32_t time;       // +0  // время измерения
  uint16_t amplitude;  // +4  // амплитуда измерения
} typeDEPADRP_Rec;   // 6


/**
 * @brief  рашаренная память для передачи данных
 */
typedef union
{
  uint8_t array[128];
  struct
  {
    typeFrameStruct frame;                // +0
    uint16_t current_meas_interval;       // +64
    typeDEPADRP_Rec matrix_rec;           // +66
    typeDEPADRPWindowChannel window_rec;  // +72
    uint8_t reserve[44];                  // +84
  } fd;
} type_DEP_Shared_vol;                    // 128

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
    int8_t Temp_dep1;      //+3
    uint8_t Dvalue_dep1;      //+2
    uint16_t Evalue_dep2;     //+4
    int8_t Temp_dep2;      //+7
    uint8_t Dvalue_dep2;      //+6
}typeDEPAcqValue;             //8

/** 
  * @brief  структура кадр ДEП-АД через МКО, в виде, как передает ДЭП-АД
  */
typedef union{
  typeFrameStruct raw;
  struct{
    uint16_t header[5];
    //
    typeDEPAcqValue meas[6]; //+10-57 5-28
    uint8_t reserve[4]; //+58
    //
    uint16_t crc16; //+62
  } dep;
}typeDepAdFrameUnion;

/** 
  * @brief  структура кадр РП ДEП-АД через МКО, в виде, как передает ДЭП-АД
  */
typedef union{
  typeFrameStruct raw;
  struct{
    uint16_t header[5];   // +0
    //
    uint16_t value;       // +10
    uint8_t reserve[50];  // +12
    //
    uint16_t crc16;       // +62
  } dep;
}typeDEP_AD_Peripheral_RP_FrameUnion; // 64

/** 
  * @brief  структура кадр РП ДEП-АД через МКО, в том виде, как его сохраняет ЦМ
  */
typedef union{
  typeFrameStruct raw;
  struct{
    uint16_t header[5];             // +0
    //
    typeDEPADRP_Rec matrix_val[8];  // +10
    uint16_t offset;                // +58
    uint8_t reserve[2];             // +60
    //
    uint16_t crc16;                 // +62
  } dep;
}typeDEP_AD_RP_FrameUnion;          // 64

/** 
  * @brief  структура кадр помеховых окон ДEП-АД через МКО, в виде, как передает ДЭП-АД
  */
typedef union{
  typeFrameStruct raw;
  struct{
    uint16_t header[5];   // +0
    //
    typeDEPADRPWindowChannel win_rec;  // +10
    uint8_t reserve[40];  // +22
    //
    uint16_t crc16;       // +62
  } dep;
}typeDEP_AD_SELF_RP_Win_FrameUnion; // 64

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
  typeIBStruct* ib_ptr;
  typeMKOBCStruct* mko_bc_ptr;
  // сfg
	uint16_t id;			          // id на внутренней шине или адрес МКО
	uint16_t self_num;          // номер устройства с точки зрения ЦМ
	uint16_t device_number, frame_type;  //параметры прибора, в котором он используется
  uint16_t rp_frame_type;
  uint16_t interval_ms;
  uint16_t const_mode;
  uint32_t *global_frame_num_ptr;
  type_DEP_Shared_vol *sh_vol;
  // выбор режима
  uint8_t mode;
  // to task_planner
  uint8_t meas_event_num;
  uint64_t last_call_time_us;
  // data
  typeDEPFrameUnion frame, mko_frame;
  uint8_t frame_data_ready;  // флаг готовности данных в памяти на отправку в другой процесс
  typeDEPProcess ch[2];
  typeDEPAcqValue last_meas;
  float last_field[2], last_freq[2];
  // mko data
  typeDepAdFrameUnion dep_ad_frame;
  // fifo для обработки данных ДЭП
  typeDEPAcqValue rec_fifo[DEP_REC_FIFO_DEPTH];
  uint8_t rec_num, rec_max;
  uint16_t current_meas_interval;  // данные для хранения измерительного интервала
  uint16_t rec_num_to_meas_int_change;  // данные для хранения измерительного интервала
  // rp_work
  typeDEP_AD_Peripheral_RP_FrameUnion per_rp_frame;
  typeDEP_AD_SELF_RP_Win_FrameUnion per_rp_win_frame;
  typeDEPADRP_Rec matrix_val_arr[8];
  typeDEPADRP_Rec matrix_raw;
  typeDEPADRPWindowChannel win_raw;
  uint8_t win_flag_ready;
  uint16_t current_offset;
  uint8_t rp_polling_ena, dep_max_survey_ena;
  typeDEP_AD_RP_FrameUnion rp_frame, mko_rp_frame;
  //
	typeFrameStruct frames_fifo[DEP_FRAME_FIFO_DEPTH];
	uint8_t frames_fifo_num, frames_fifo_num_max;
	uint32_t fifo_error_cnt;
  //
  uint16_t offset_to_set;
  uint8_t bound_to_set[3];
  // general
	
  // cyclogram_ctrl
  typeCyclograma meas_cyclo;
} typeDEPStruct;

#pragma pack(pop)

//
void dep_init(  typeDEPStruct* dep_ptr, 
                uint8_t self_num, 
                uint8_t id, 
                uint16_t device_number, 
                uint16_t frame_type, 
                uint16_t rp_frame_type, 
                typeIBStruct* ib_ptr, 
                typeMKOBCStruct* mko_bc_ptr, 
                uint32_t* gl_fr_num, 
                uint8_t mode,
                uint32_t offset);
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
void dep_read_rp_data(typeDEPStruct *dep_ptr);
void dep_read_win_data(typeDEPStruct *dep_ptr);
void dep_bound_set(typeDEPStruct *dep_ptr, uint16_t win_num, uint16_t bound_val);
void dep_rp_offset_set(typeDEPStruct *dep_ptr, uint16_t offset_val);

int32_t dep_rp_set_win_bnd(typeDEPStruct *dep_ptr, uint8_t bnd_1, uint8_t bnd_2, uint8_t bnd_3);
void dep_rp_matrix_process(typeDEPStruct *dep_ptr);
void dep_rp_frame_forming(typeDEPStruct* dep_ptr);
void dep_rp_mko_frame_forming(typeDEPStruct* dep_ptr);

float __dep_field_calculate(uint16_t raw_data);
float __dep_freq_calculate(uint8_t raw_data);

int8_t dep_write_fifo(typeDEPStruct *dep_ptr, typeDEPAcqValue* data);
int8_t dep_read_fifo(typeDEPStruct *dep_ptr, typeDEPAcqValue* data);
int8_t dep_frame_write_fifo(typeDEPStruct* dep_ptr, typeFrameStruct* frame);
int8_t dep_frame_read_fifo(typeDEPStruct* dep_ptr, typeFrameStruct* frame);

// функции для работы циклограмы измерительного интервала
void dep_meas_cycl_init(typeDEPStruct* dep_ptr);
int32_t dep_meas_cycl_struct_start(void* ctrl_struct, uint8_t* data);
int32_t dep_meas_cycl_struct_stop(void* ctrl_struct, uint8_t* data);
int32_t dep_meas_cycl_read_data(void* ctrl_struct, uint8_t* data);
int32_t dep_meas_cycl_frame_forming(void* ctrl_struct, uint8_t* data);
int32_t dep_meas_cycl_rp_form_data(void* ctrl_struct, uint8_t* data);
int32_t dep_meas_cycl_rp_read_win_data(void* ctrl_struct, uint8_t* data);
int32_t dep_meas_cycl_rp_set_win_bnd(void* ctrl_struct, uint8_t* data);
int32_t dep_meas_cycl_rp_set_offset(void* ctrl_struct, uint8_t* data);
int32_t dep_meas_cycl_do_nothing(void* ctrl_struct, uint8_t* data);


#endif
