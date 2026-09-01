#ifndef _MCI_H_
#define _MCI_H_

#include "main.h"
#include <stdio.h>
#include <string.h>
#include "gpio.h"
#include "frames.h"
#include "task_planner.h"
#include "cyclogramma.h"

#define     MCI_BUFF_FRAMES             (7)
#define     MCI_TRANS_TIMEOUT_MS        (6)

#define     MCI_PROCESS_PERIOD          (1)

/** 
  * @brief  буфер для подготовки данных для передачи данных по интерфейсу МЦИ
  */
typedef union{
    struct{
        uint8_t header[10];
        typeFrameStruct frame[MCI_BUFF_FRAMES];
        uint8_t reserve[44];
    } fd;
    uint8_t array[502];
}type_MCI_FRAME_BUFFER;

/** 
  * @brief  циклический буфер на базе буфера для МЦИ
  */
typedef union{
    type_MCI_FRAME_BUFFER buffer;
    uint16_t rd_ptr;
}type_MCI_FRAME_FIFO;

/**
 * @brief  структура управления МПП
 */
typedef struct
{
    // Inst
    SPI_TypeDef* spi_inst[2];
    // сfg
    type_MCI_FRAME_FIFO buffer;
    uint64_t timeout_us[2];
    uint8_t tx_ena[2];
    uint32_t transaction_timeout[2];
    //
    uint64_t last_call_time_us, call_interval_us;
} typeMCIStruct;

void mci_init(typeMCIStruct *mci_ptr);
void mci_reset_parameters(typeMCIStruct *mci_ptr, uint8_t spi_num);
int8_t mci_process_tp(void* ctrl_struct, uint64_t time_us, typeProcessInterfaceStruct* interface);

void mci_clear_fifo(typeMCIStruct *mci_ptr);

#endif

