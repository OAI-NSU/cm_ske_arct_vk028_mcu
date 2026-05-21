#ifndef _UART_H_
#define _UART_H_

#include "main.h"
#include "CM4/K1921VK028.h"
#include "system_K1921VK028.h"

#define UART_RX_BUFF_SIZE     512    
#define UART_TX_BUFF_SIZE     512
#define FRAME_GAP_RX_US       1800
#define TIMER_LOAD_US         100
#define FRAME_GAP_RX_CNT      (FRAME_GAP_RX_US / TIMER_LOAD_US)
#define FRAME_GAP_TX_US       2500
#define FRAME_GAP_TX_CNT      (FRAME_GAP_TX_US / TIMER_LOAD_US)


void UART_Init(UART_TypeDef *pUART_ref, uint32_t baudrate);
int UART_Rx(UART_TypeDef *pUART_ref, uint8_t *bt);
int UART_RxPacket(UART_TypeDef *pUART_ref, uint8_t *buff, uint16_t buff_size);
int8_t UART_RxFullPacket(UART_TypeDef *pUART_ref, uint8_t *buff, uint16_t* rx_len);
void UART_Tx(UART_TypeDef *pUART_ref, uint8_t bt);
void UART_TxPacket(UART_TypeDef *pUART_ref, uint8_t *buff, uint16_t leng);
int8_t UART_PacketInWaiting(UART_TypeDef *pUART_ref);
int8_t UART_PacketReady(UART_TypeDef *pUART_ref);

#endif
