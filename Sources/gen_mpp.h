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
#define GMPP_REC_FIFO_DEPTH                   (16)

#define GMPP_DEFAULT_INTERVAL_MS              (1000)

#define GMPP_EVENT_MEAS_INTERVAL_START        (1<<0)
#define GMPP_EVENT_MEAS_INTERVAL_DATA_READY   (1<<1)

#define GMPP_EVENT_MATRIX_REC_DATA_READY      (1 << 7)
#define GMPP_EVENT_WINDOW_REC_DATA_READY      (1 << 8)

#define GMPP_DEVICE_NUM                       (16)  // количество устройств всего в аппаратуре

#define GMPP_USED_DEVICE_SIZE                 (10)  // количество устройств, которые определяют размер матрицы

#define GMPP_MATRIX_REC_SIZE                  (((GMPP_USED_DEVICE_SIZE + 1) & ~1) + 2)
#define GMPP_MATRIX_LINE_NUM                  (52 / GMPP_MATRIX_REC_SIZE)   // количество строк матрицы на один кадр
#define GMPP_MATRIX_FRAME_NUM                 \
      ((GMPP_USED_DEVICE_SIZE + GMPP_MATRIX_LINE_NUM - 1) / GMPP_MATRIX_LINE_NUM)   // количество кадров для помеховой матрицы
#define GMPP_MATRIX_RESERVED_SIZE            \
    (52 - GMPP_MATRIX_LINE_NUM * GMPP_MATRIX_REC_SIZE)

#define GMPP_WIN_REC_SIZE                     (12)
#define GMPP_WIN_REC_NUM                      (52 / GMPP_WIN_REC_SIZE)   // количество строк матрицы на один кадр
#define GMPP_WIN_FRAME_NUM                    \
      ((GMPP_USED_DEVICE_SIZE + GMPP_WIN_REC_NUM - 1) / GMPP_WIN_REC_NUM)   // количество кадров для помеховой матрицы
#define GMPP_WIN_RESERVED_SIZE                \
    (52 - GMPP_WIN_REC_NUM * GMPP_WIN_REC_SIZE)

#define GMPP_DEVICE_WORK_MASK_DEFAULT         (0x3F3)

#if GMPP_MATRIX_RESERVED_SIZE < 0
#error "Matrix rows exceed the 52-byte frame payload"
#endif

#if GMPP_WIN_RESERVED_SIZE < 0
#error "Window rows exceed the 52-byte frame payload"
#endif

#pragma pack(push, 1)

/**
 * @brief исходное измерение канала для формирования помеховой матрицы
 */
typedef struct
{
  uint32_t time;       // +0  // время измерения
  uint16_t amplitude;  // +4  // амплитуда измерения
} typeGMPPMatrixVal;   // 6

/**
 * @brief значение оконных параметров канала МПП
 *
 */
typedef struct
{
  uint16_t bound[3];      // +0  // границы трёх окон
  uint16_t num[3];        // +6  // счётчики трёх окон
} typeGMPPWindowChannel;  // 12

/**
 * @brief строка помеховой матрицы с полным временем события и полным количеством устройств
 * @note используется при накоплении данных и вычислении времени события
 */
typedef struct
{
  uint8_t  amplitude[GMPP_DEVICE_NUM];  // амплитуды участников матрицы
  uint32_t time;                         // полное время события
} typeMPPFullMatrixString;

/**
 * @brief строка помеховой матрицы с двухбайтовым полем времени
 * @note сокращённый формат для размещения данных в кадре
 */
typedef struct
{
  uint8_t  amplitude[GMPP_USED_DEVICE_SIZE];  // амплитуды участников матрицы
  uint16_t time;                         // сокращённое представление времени события
} typeMPPRestrictedMatrixString;

/**
 * @brief кадр помеховой матрицы в общем и структурированном представлениях
 */
typedef union
{
  typeFrameStruct raw;  // общий формат кадра
  struct {
    uint16_t            label;                                // +0  // метка начала кадра
    uint16_t            definer;                              // +2  // идентификатор кадра
    uint16_t            num;                                  // +4  // номер кадра
    uint32_t            time;                                 // +6  // время формирования кадра
    typeMPPRestrictedMatrixString matrix_string[GMPP_MATRIX_LINE_NUM];  // строки помеховой матрицы
#if GMPP_MATRIX_RESERVED_SIZE > 0
    uint8_t             reserved[GMPP_MATRIX_RESERVED_SIZE];  // резервные байты кадра
#endif
    uint16_t            crc16;                                // контрольная сумма кадра
  } mpp;
} typeMPPMatrixFrameUnion;  // 64

/**
 * @brief кадр оконных данных в общем и структурированном представлениях
 */
typedef union
{
  typeFrameStruct raw;  // общий формат кадра
  struct {
    uint16_t                  label;              // +0  // метка начала кадра
    uint16_t                  definer;            // +2  // идентификатор кадра
    uint16_t                  num;                // +4  // номер кадра
    uint32_t                  time;               // +6  // время формирования кадра
    typeGMPPWindowChannel     window_channel[GMPP_WIN_REC_NUM];  // оконные данные восьми каналов
    uint8_t                   reserved[GMPP_WIN_RESERVED_SIZE];        // резервные байты кадра
    uint16_t                  crc16;              // контрольная сумма кадра
  } mpp;
} typeMPPWindowFrameUnion;

#pragma pack(pop)

/**
  * @brief  структура управления агрегатором матричных и оконных данных
  */
typedef struct
{
  uint16_t                self_num;                                                                      // номер агрегатора в списке устройств ЦМ
  uint16_t                device_number, win_frame_type, mtrx_frame_type;                                // номер прибора и типы оконных и матричных кадров
  uint32_t                *global_frame_num_ptr;                                                         // указатель на общий счётчик кадров

  uint8_t                 device_list_id[GMPP_DEVICE_NUM];                                               // номера источников данных в списке устройств ЦМ
  uint32_t                mpp_matrix_time_error;                                                         // счётчик ошибок согласования времени матрицы
  
  uint32_t                gmpp_used_device_flag;
  uint8_t                 redirection_array[GMPP_USED_DEVICE_SIZE];  // индексы выбранных устройств; 0xFF для свободных позиций
  uint8_t                 redirection_count;                        // количество выбранных устройств

  typeGMPPMatrixVal       matrix_rec[GMPP_DEVICE_NUM];                                                   // исходные измерения для матрицы
  typeMPPFullMatrixString mpp_mtrx_str;                                                                  // текущая строка амплитуд с полным временем
  typeMPPFullMatrixString mpp_mtrx_array[GMPP_DEVICE_NUM];                                               // накопленные строки матрицы
  typeMPPMatrixFrameUnion matrix_frame[GMPP_MATRIX_FRAME_NUM], mko_matrix_frame[GMPP_MATRIX_FRAME_NUM];  // матричные кадры до и после преобразования для МКО
  
  typeGMPPWindowChannel   win_rec[GMPP_DEVICE_NUM];                                                      // исходные оконные данные устройств
  typeMPPWindowFrameUnion win_frame[GMPP_WIN_FRAME_NUM], mko_win_frame[GMPP_WIN_FRAME_NUM];              // оконные кадры до и после преобразования для МКО

  uint8_t                 win_mpp_flag;                                                                  // флаг обработки оконных данных
  uint8_t                 matrix_mpp_flag;                                                               // флаг обработки матричных данных
  uint32_t                win_mpp_cnt;                                                                   // счётчик обработки оконных данных
  uint32_t                matrix_mpp_cnt;                                                                // счётчик обработки матричных данных
  typeFrameStruct         frames_fifo[GMPP_REC_FIFO_DEPTH];                                              // очередь кадров для передачи

  uint8_t                 frames_fifo_num, frames_fifo_num_max;                                          // текущее и максимальное заполнение очереди
  uint32_t                fifo_error_cnt;                                                                // счётчик ошибок очереди

  uint64_t                last_call_time_us, call_interval_us;                                           // время последнего вызова и интервал между вызовами, мкс
} typeGEN_MPP_Struct;

void gmpp_init( typeGEN_MPP_Struct* gmpp_ptr,
                uint8_t self_num,
                uint16_t device_number,
                uint16_t mtrx_frame_type,
                uint16_t win_frame_type,
                uint32_t* gl_fr_num);
void gmpp_reset_parameters(typeGEN_MPP_Struct* gmpp_ptr);
int8_t gmpp_process_tp(void* ctrl_struct, uint64_t time_us, typeProcessInterfaceStruct* interface);

void gmpp_matrix_process_data(typeGEN_MPP_Struct* gmpp_ptr);
void gmpp_matrix_clear_data(typeGEN_MPP_Struct* gmpp_ptr);
void gmpp_matrix_clear_rec(typeGEN_MPP_Struct* gmpp_ptr);
void gmpp_win_clear_data(typeGEN_MPP_Struct* gmpp_ptr);
void gmpp_set_redirection_array(typeGEN_MPP_Struct* gmpp_ptr, uint32_t redirection_mask);
uint8_t gmpp_get_device_index(const typeGEN_MPP_Struct* gmpp_ptr, uint8_t ordinal);
// Общая нумерация для матрицы и окон: ordinal с 0; результат 0xFF при отсутствии устройства.
uint8_t gmpp_get_device_index(const typeGEN_MPP_Struct* gmpp_ptr, uint8_t ordinal);

void gmpp_matrix_frame_forming(typeGEN_MPP_Struct* gmpp_ptr);
void gmpp_matrix_mko_frame_forming(typeGEN_MPP_Struct* gmpp_ptr);
void gmpp_win_frame_forming(typeGEN_MPP_Struct* gmpp_ptr);
void gmpp_win_mko_frame_forming(typeGEN_MPP_Struct* gmpp_ptr);

void gmpp_fifo_init(typeGEN_MPP_Struct* gmpp_ptr);
int8_t gmpp_write_fifo(typeGEN_MPP_Struct* gmpp_ptr, typeFrameStruct* frame);
int8_t gmpp_read_fifo(typeGEN_MPP_Struct* gmpp_ptr, typeFrameStruct* frame);

#endif
