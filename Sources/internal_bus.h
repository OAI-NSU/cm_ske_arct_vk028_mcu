#ifndef _INTERNAL_BUS_H_
#define _INTERNAL_BUS_H_

#include <string.h>
#include "main.h"
#include "CM4/K1921VK028.h"
#include "system_K1921VK028.h"
#include "uart.h"
#include "timers.h"
#include "task_planner.h"

// дефайны для переменных
#define IB_UART_BAUDRATE                  125000

#define IB_STARTUP_TIMEOUT_MS             5000
#define IB_PROCESS_PERIOD                 100
//
#define IB_UART_TIMEOUT_MS							  15

#define IB_FRAME_TYPE_RX                  0x01
#define IB_FRAME_TYPE_TX                  0x02
#define IB_FRAME_TYPE_ERROR               0x03

#define IB_GLOBAL_DBG_DEFAULT           (0x00)

#define MB_DEV_ID_NU                      0x0F  //для отправки отладочной информации

#define MB_F_CODE_IDLE									  0x00
#define MB_F_CODE_3											  0x03
#define MB_F_CODE_6											  0x06
#define MB_F_CODE_16										  0x10
#define MB_F_CODE_BROAD									  0xFF
#define MB_F_CODE_106										  0x6A  // не в стандарте ModBus
#define MB_F_CODE_111										  0x6F  // не в стандарте ModBus
#define MB_F_CODE_ERROR									  0x80

#define MB_FRAME_TYPE_RX                  0x01
#define MB_FRAME_TYPE_TX                  0x02
#define MB_FRAME_TYPE_ERROR               0x03

#define MB_CRC16_INIT_VAL                 0xFFFF

#define MB_UART_RX_STATUS_OK							1
#define MB_UART_RX_STATUS_EXCEPTION				0
#define MB_UART_RX_STATUS_LENGTH					-1
#define MB_UART_CRC_ERROR									-2
#define MB_UART_UNEXPECTED_FRAME					-3

#define MB_TRANSACTION_WRITE_BROAD	      0x03
#define MB_TRANSACTION_WRITE				      0x02
#define MB_TRANSACTION_READ					      0x01
#define MB_TRANSACTION_IDLE 				      0x00

#define MB_BUFFER_SIZE                    128


#pragma pack(push, 2)

/** 
  * @brief  структура-интерфейс к UART-периферии
  * @param  init  функция инициализации UART устройства
  * @param  send_packet  функция отправки пакета, en_crc: 1 - подсчет и добавление crc16-ModBus, 0-ничего
  * @param  get_packet  функция приема пакета, возвращает 0 - нет пакета, 1 - есть пакет с правильным crc16, -1 - прием пакета с некорректным crc16
  */
typedef struct
{
  void (*init) (UART_TypeDef *pUART_ref, uint32_t baudrate);
  void (*send_packet) (UART_TypeDef *pUART_ref, uint8_t* buff, uint16_t buff_size);
  int8_t (*get_packet) (UART_TypeDef *pUART_ref, uint8_t* buff, uint16_t* rx_len);
  int8_t (*packet_in_waiting) (UART_TypeDef *pUART_ref);
  int8_t (*packet_ready) (UART_TypeDef *pUART_ref);
} typeUARTInterface;

/** 
  * @brief  структура-заготовка для пакетов ModBus
  */
typedef struct{
	uint8_t dev_id;
	uint8_t f_code;
	uint16_t reg_addr;
	uint16_t reg_cnt;
	uint8_t byte_cnt;
	uint16_t data[MB_BUFFER_SIZE];
	uint16_t crc_16;
	uint8_t error_code;
	uint8_t type;
} typeModBusFrameStruct;

/** 
  * @brief  структура управления внутренней шиной (ВШ, IB - internal bus)
  */
typedef struct
{
  // псевдоним
  char alias[32];           //! псевдоним канала для разбора и формирования отчетов
  //
  typeUARTInterface uart;
  UART_TypeDef *uart_inst;
  typeModBusFrameStruct rx_frame, tx_frame, command_frame;
  uint8_t command_frame_flag;
  uint32_t command_counter;
  uint8_t tx_data[MB_BUFFER_SIZE];
  uint16_t tx_len;
  uint32_t tx_counter;
	uint8_t rx_data[MB_BUFFER_SIZE];
  uint16_t rx_len;
  uint32_t rx_counter;
  uint32_t error_status, nans_status;
  uint8_t error_counter, nans_counter;
  uint8_t transaction_type;
  uint8_t global_dbg_flag;
  //
  uint64_t last_call_time_us;
  uint64_t call_interval_us;
} typeIBStruct;

#pragma pack(pop)

//
void ib_init(typeIBStruct* ib_ptr);
int8_t ib_process_tp(void* ctrl_struct, uint64_t time_us, typeProcessInterfaceStruct* interface);
void ib_reset_parameters(typeIBStruct* ib_ptr);
void ib_reset_state(typeIBStruct* ib_ptr);
int8_t ib_run_transaction(typeIBStruct* ib_ptr, uint8_t dev_id, uint8_t f_code, uint16_t reg_addr, uint16_t reg_cnt, uint16_t* data);
uint8_t ib_get_answer_data(typeIBStruct* ib_ptr, uint8_t* buff, uint8_t len);
int8_t __ib_parsing_rx_data(typeIBStruct* ib_ptr, uint8_t* data, uint8_t leng, uint8_t transaction_type);
//
void mb_frame_form_packet(typeModBusFrameStruct* frame_ptr, uint8_t* data, uint16_t* leng);
uint16_t mb_frame_calc_crc16(typeModBusFrameStruct* frame_ptr);

//
uint16_t __modbusb_crc16(uint8_t *data, uint16_t len, uint16_t crc16_init);
//

#endif
