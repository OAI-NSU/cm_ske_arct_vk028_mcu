#ifndef _FLIGHT_TASK_H_
#define _FLIGHT_TASK_H_

//*** Includes ***//
#include "byteswap.h"
#include "crc16.h"
#include "task_planner.h"
#include "mka_frames.h"

//*** Defines ***//
#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

// определитель кадров (метка для визуального или автоматического поиска кадров)
#define FT_MARK                    0xFAFB
// параметры полетных заданий
#define FT_STEP_LEN_BYTE    (64)  //необходимо проверять длину шага
#define FT_LEN_STEP         (64)
#define FT_LEN_BYTE         (FT_LEN_STEP*FT_STEP_LEN_BYTE)

enum FT_MODE
{
  FT_MODE_OFF, FT_MODE_START, FT_MODE_WORK, FT_MODE_PAUSE, 
  FT_MODE_NUM
};

#define FT_FUNC_GROUP_NUM_MAX           (16)
#define FT_FUNCT_NUM_MAX                (32)

// настройки FIFO для полетного задания
#define FT_FIFO_DEPTH                   (4)  //! глубина FIFO в кадрах
#define FT_FIFO_REC_LEN                 (128)  //! длина единичной записи

#define FT_DATA_TO_SAVE_MAX_LEN         (128)

// настройки шага в 
#define FT_STATUS_SAVE_DATA_ENA         (1<<0)  //! сохранение данных в fifo (всю логику сохранения в виде кадров переносим в функции сохранения, оставляя модуль полетного задания независимым от формата кадра)
#define FT_STATUS_RSRV                  (1<<1)  //! NU
#define FT_STATUS_BRANCH_RPT_BREAK      (1<<2)  //! ветвление по статусу выполнения функции: выход из повтора
#define FT_STATUS_BRANCH_FT_BREAK       (1<<3)  //! ветвление по статусу выполнения функции: выход из полетного задания
#define FT_STATUS_BRANCH_GO_TO_INH      (1<<4)  //! ветвление по статусу выполнения функции: отмена перехода по GO_TO

#define FT_FUN_RET_NOPE                 (0)

// статусы работы FT
#define FT_STATUS_CLEAR                 (0)
#define FT_STATUS_WORK                  (1<<0)
#define FT_STATUS_PAUSE                 (1<<1)
#define FT_STATUS_OTHER_ERROR           (1<<2)
#define FT_STATUS_STEP_ERROR            (1<<3)
#define FT_STATUS_CRC_ERROR             (1<<4)
#define FT_STATUS_LUNCH_ERROR           (1<<5)
#define FT_STATUS_WR_TO_ROM_ERROR       (1<<6)
#define FT_STATUS_WR_TO_RAM_ERROR       (1<<7)
#define FT_STATUS_HEADER_ERROR          (1<<8)
#define FT_STATUS_FUNC_ERROR            (1<<9)
#define FT_STATUS_FIFO_ERROR            (1<<10)

#define FT_STATUS_SET                   (1)
#define FT_STATUS_RELEASE               (0)


//*** General parts of frames ***//
#pragma pack(push, 1)


typedef union{
  uint8_t array[14];
  struct {
    uint8_t self_id;        // +0
    uint8_t mode;           // +1
    uint8_t step_num;       // +2
    uint8_t function_type;  // +3
    uint8_t function_cmd;   // +4    
    uint8_t save_cnt;       // +5
    uint16_t rpt_value;     // +6
    uint16_t status;        // +8
    uint8_t lunch_cnter;    // +10
    uint8_t err_cnter;      // +11
    uint8_t reserve[2];     // +12
  }fields;
}typeFTReport;  // 14

typedef union{
  uint8_t array[4];
  struct {
    uint8_t mode;
    uint8_t step_num;
    uint8_t function_type;
    uint8_t function_cmd;    
  }fields;
}typeFT_Short_Report;  // 4

/**
  * @brief  шаг полетного задания длина 64 байта фиксирована
  */
typedef union{
  uint8_t array[FT_STEP_LEN_BYTE];
  struct {
    uint16_t label;         // +0
    uint16_t type;           // +2
    uint16_t cmd;            // +4
    uint16_t rpt_cnt;       // +6
    uint16_t go_to;          // +8
    uint16_t go_cnt;         // +10
    uint32_t delay_ms;      // +12
    uint16_t settings;      // +16
    uint16_t reserve[6];    // +18
    uint8_t data[32];       // +30
    uint16_t crc16;         // +62
  }fields;
}typeFTStep;  // 64

/**
  * @brief  массив полетного задания
  */
typedef union
{
  typeFTStep  step[FT_LEN_STEP];
  uint8_t     array[FT_LEN_BYTE];
}typeFTArray;

/**
  * @brief  функции работы полетного задания
  */
typedef struct
{
  int32_t (*func) (void* ft_ptr, void* ctrl_struct, uint8_t* ctrl_data, uint8_t* data_to_save, uint8_t* data_len);
  void* ctrl_struct;
}typeFTFunction;

/**
  * @brief  контейнер функций для работы полетного задания
  */
typedef struct
{
  typeFTFunction func[FT_FUNCT_NUM_MAX];
}typeFTFunctionGroup;

typedef struct{
  typeFTStep step_cast; //! слепок текущего шага
  // для циклирования используются переходы и счетчики переходов
  uint16_t go_to_cnt[FT_LEN_STEP]; //! глобальный счетчик переходов между шагами
  uint16_t go_to_max[FT_LEN_STEP]; //! значение переходов шагов всего полетного задания
  // управление шагом
  uint64_t last_step_time;
  uint32_t step_delay;
  uint32_t pause_time, last_pause_time;
  uint8_t step_num, step_num_prev;
  uint16_t rpt_value;
  // параметры вызова функций
  uint8_t data_to_save[FT_DATA_TO_SAVE_MAX_LEN], data_len;
  //
  uint16_t save_cnt;
  //
  typeSingleFrameStruct ft_frame;
  //
  int32_t func_ret_val;
} typeFTCtrl;

typedef struct{
  uint8_t array[FT_FIFO_DEPTH][FT_FIFO_REC_LEN];
  uint8_t rec_num, rec_max;
} typeFTFifo;


typedef struct{
  uint64_t cnter;
  uint32_t bnd_val;
  uint8_t status;
} type_FT_Tim;

/**
  * @brief  
  */
typedef struct
{
  // общие настройки
  uint8_t self_num, sat_id, dev_id;
  typeFTArray task;
  //
  char name[16];
  //
  uint8_t mode;
  uint8_t print_ena;
  uint16_t status;
  uint8_t start_cnter;
  uint8_t error_cnt;
  // вызываемые функции
  typeFTFunctionGroup f_group[FT_FUNC_GROUP_NUM_MAX];
  // управление работой циклограммы
  typeFTCtrl ctrl;
  // таймер
  type_FT_Tim timer[8];
  //
  typeFTReport report;
  typeFT_Short_Report short_report;
  // fifo для хранения данных 
  typeFTFifo fifo;
  // текущее управление
  uint64_t last_call_time_ms;
  uint64_t last_call_period_ms;
}typeFT;

#pragma pack(pop)

//*** Function prototypes ***//

int8_t ft_init(typeFT *ft_ptr, char* name, uint8_t self_num, uint16_t sat_id, uint8_t dev_id);
void ft_reset_status(typeFT *ft_ptr);
int8_t ft_process_tp(void* ctrl_struct, uint64_t time_us, typeProcessInterfaceStruct* interface);
int8_t ft_process(typeFT *ft_ptr, uint64_t time_ms);
void ft_mode_work_process(typeFT *ft_ptr, uint64_t time_ms);
int8_t ft_create_step_cast(typeFT *ft_ptr);
int32_t ft_func_run(typeFT* ft_ptr);
int32_t ft_function_retval_check(typeFT* ft_ptr);
int8_t ft_check_step_cast_crc(typeFT *ft_ptr);
void __ft_step_cast_reset(typeFT *ft_ptr);
int8_t ft_check_step_cast_crc(typeFT *ft_ptr);
int8_t ft_function_registration(typeFT *ft_ptr, uint8_t group_num, uint8_t func_num, int32_t (*func) (void*, void*, uint8_t*, uint8_t*, uint8_t*), void* ctrl_struct);
int8_t ft_set_mode(typeFT *ft_ptr, uint8_t mode);
uint8_t ft_get_operation_status(typeFT *ft_ptr);
uint8_t ft_get_error_status(typeFT *ft_ptr);
int8_t ft_load_task(typeFT *ft_ptr, uint8_t *task_array);
void ft_refresh_go_to_parameters(typeFT *ft_ptr);
typeFTArray ft_unload_task(typeFT *ft_ptr);
int8_t ft_check_task(typeFT *ft_ptr);
int8_t ft_write_fifo(typeFT *ft_ptr, uint8_t* data);
int8_t ft_read_fifo(typeFT* ft_ptr, uint8_t* data);
typeFTReport ft_create_report(typeFT* ft_ptr);
typeFT_Short_Report ft_create_short_report(typeFT* ft_ptr);
void ft_set_print_mode(typeFT *ft_ptr, uint8_t mode);
//
void ft_create_ft_step(typeFTStep* step, uint16_t type, uint16_t cmd, uint16_t rpt_cnt, uint32_t delay_ms, uint16_t settings, uint16_t go_to, uint16_t go_cnt, uint8_t*data);
//
int32_t ft_def_fun_do_nothing(void* ft_ptr, void* ctrl_struct, uint8_t* ctrl_data, uint8_t* data_to_save, uint8_t* data_len);
int32_t ft_def_fun_start_timer_with_bnd(void* ft_ptr, void* ctrl_struct, uint8_t* ctrl_data, uint8_t* data_to_save, uint8_t* data_len);
int32_t ft_def_fun_check_timer(void* ft_ptr, void* ctrl_struct, uint8_t* ctrl_data, uint8_t* data_to_save, uint8_t* data_len);
int32_t ft_def_fun_start_timer_without_bnd(void* ft_ptr, void* ctrl_struct, uint8_t* ctrl_data, uint8_t* data_to_save, uint8_t* data_len);
int32_t ft_def_fun_check_timer_by_ctrl_data(void* ft_ptr, void* ctrl_struct, uint8_t* ctrl_data, uint8_t* data_to_save, uint8_t* data_len);
//
void  ft_status_collector(typeFT* ft_ptr, uint16_t flag, uint8_t type);
//
uint16_t ft_crc16(uint8_t *buf, uint8_t len);

#endif
