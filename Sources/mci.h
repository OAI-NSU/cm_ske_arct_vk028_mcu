#ifndef _MCI_H_
#define _MCI_H_

#include "main.h"
#include <stdio.h>
#include <string.h>
#include "gpio.h"
#include "frames.h"
#include "task_planner.h"
#include "cyclogramma.h"
#include "clock.h"

#define     MCI_DATA_FRAME_SIZE         (512)
#define     MCI_BUFF_FRAMES             (7)
#define     MCI_TRANS_TIMEOUT_MS        (10)
#define     MCI_TRANS_STEP_MS           (2)

#define     MCI_PROCESS_PERIOD          (1)

#define     MCI_HEADER_LABEL            {0x00, 0x0E, 0x0E, 0x00, 0x00, 0x00}

#define     MCI_FIFO_DEPTH              (14)

//структуры кадров
#pragma pack(push, 1)

/**
 * @brief Заголовок буфера для передачи по МЦИ
 * 
 */
typedef struct{
    uint8_t mci_label[6];
    uint32_t time_s;
}type_MCI_Buff_Header;

/** 
  * @brief  буфер для подготовки данных для передачи данных по интерфейсу МЦИ
  */
typedef union{
    struct{
        type_MCI_Buff_Header header;
        typeFrameStruct frame[MCI_BUFF_FRAMES];
        uint8_t reserve[MCI_DATA_FRAME_SIZE-10-MCI_BUFF_FRAMES*64];
    } fd;
    uint8_t array[MCI_DATA_FRAME_SIZE];
    uint16_t arr_u16[MCI_DATA_FRAME_SIZE/2];
}type_MCI_FRAME_BUFFER;

#pragma pack(pop)

/** 
  * @brief  циклический буфер на базе буфера для МЦИ
  */
typedef struct{
    type_MCI_FRAME_BUFFER buffer;
    uint16_t rd_ptr;
}type_MCI_Data_Frame;

/**
 * @brief fifo для формирования потока кадров
 * 
 */
typedef struct{
    typeFrameStruct rec_fifo[MCI_FIFO_DEPTH];
    uint8_t rec_num, rec_max;
}type_MCI_Frame_FIFO;

/**
 * @brief  структура управления МПП
 */
typedef struct
{
    // Inst
    SPI_TypeDef* spi_inst[2];
    // сfg
    type_MCI_Data_Frame buffer[2];
    type_MCI_Frame_FIFO fifo;
    uint64_t timeout_us[2];
    uint8_t tx_ena[2];
    uint8_t reinit_done[2];
    uint32_t transaction_cnter[2];
    uint32_t transaction_bit_cnter[2][8];
    uint32_t transaction_bit_cnter_num[2];
    //
    uint64_t last_call_time_us, call_interval_us;
} typeMCIStruct;

void mci_init(typeMCIStruct *mci_ptr);
void mci_reinit_spi(typeMCIStruct *mci_ptr, uint8_t spi_num);
int8_t mci_process_tp(void* ctrl_struct, uint64_t time_us, typeProcessInterfaceStruct* interface);

uint8_t mci_tx_preparation(typeMCIStruct *mci_ptr, uint64_t interval_us);
void mci_fill_buf(typeMCIStruct *mci_ptr, uint8_t spi_num);
void mci_clean_buf(typeMCIStruct *mci_ptr, uint8_t spi_num);

void mci_clean_fifo(typeMCIStruct *mci_ptr);
int8_t mci_write_fifo(typeMCIStruct *mci_ptr, uint8_t *frame);
int8_t mci_read_fifo(typeMCIStruct *mci_ptr, uint8_t *frame);
uint8_t mci_get_fifo_state(typeMCIStruct *mci_ptr);


void mci_spi_tx_irq_handler(typeMCIStruct *mci_ptr, int spi_num);

#endif
