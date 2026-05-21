#include "uart.h"

uint8_t UARTRxBuff[4][UART_RX_BUFF_SIZE];
uint16_t UARTRxBuffTail[4], UARTRxBuffHead[4];
uint8_t UARTTxBuff[4][UART_TX_BUFF_SIZE];
uint16_t UARTTxBuffTail[4], UARTTxBuffHead[4];
volatile int UARTTxCompleteFlg[4] = {0, 0, 0, 0};
volatile int UARTForceTxIrq = 0;
volatile int FrameGapRx[4], FrameGapTx[4];

void uart_IRQHandler(UART_TypeDef *pUART_ref);

int uart_get_num(UART_TypeDef *pUART_ref) {
  int num = -1;
  switch((int)pUART_ref) {
    case (int)UART0:  num = 0;  break;  // NU
    case (int)UART1:  num = 1;  break;  // ���������� ���� 1
    case (int)UART2:  num = 2;  break;  // ���������� ���� 2
    // case (int)UART3:  num = 3;  break; - ������� ��� ��������������� printf()
    }
  return num;
}

void uart_tx_enable(UART_TypeDef *pUART_ref, int enable) {
  /*без эха*/
  if(enable)  pUART_ref->CR &= ~UART_CR_RXE_Msk;
  else  pUART_ref->CR |= UART_CR_RXE_Msk;
}

void UART_Init(UART_TypeDef *pUART_ref, uint32_t baudrate) {
  int uart_num;
  uint32_t uartcfg;
  uint32_t baud_icoef = 0;
  uint32_t baud_fcoef = 0;
  uint32_t uart_clk = 0;
  uart_num = uart_get_num(pUART_ref);
  if(uart_num < 0)  return;
  uartcfg = (1 << RCU_UARTCFG_UARTCFG_CLKSEL_Pos) |
            RCU_UARTCFG_UARTCFG_CLKEN_Msk |
            RCU_UARTCFG_UARTCFG_RSTDIS_Msk |
            (0 << RCU_UARTCFG_UARTCFG_DIVN_Pos) |
            (1 << RCU_UARTCFG_UARTCFG_DIVEN_Pos);
  switch(uart_num) {
    case 0:  RCU->UARTCFG[0].UARTCFG = uartcfg;  break;
    case 1:  
      GPIOK->DENSET = (1<<8) | (1<<9);
      GPIOK->PULLMODE |= (1<<9*2);
      GPIOK->ALTFUNCSET = (1<<8) | (1<<9);
      GPIOK->ALTFUNCNUM1 |= (4<<(8-8)*4) | (4<<(9-8)*4);
      RCU->UARTCFG[1].UARTCFG = uartcfg;  
      NVIC_EnableIRQ(UART1_IRQn);
      break;
    case 2: 
      // GPIOK->DENSET = (1<<10) | (1<<11);
      // GPIOK->PULLMODE |= (1<<11*2);
      // GPIOK->ALTFUNCSET = (1<<10) | (1<<11);
      // GPIOK->ALTFUNCNUM1 |= (4<<(10-8)*4) | (4<<(11-8)*4);
      // RCU->UARTCFG[2].UARTCFG = uartcfg;  
      // NVIC_EnableIRQ(UART2_IRQn);
      break;
    // case 3:  
    //   GPIOK->DENSET = (1<<15) | (1<<14);
    //   GPIOK->PULLMODE |= (1<<14*2);
    //   GPIOK->ALTFUNCSET = (1<<15) | (1<<14);
    //   GPIOK->ALTFUNCNUM1 |= (4<<(15-8)*4) | (4<<(14-8)*4);
    //   RCU->UARTCFG[3].UARTCFG = uartcfg;
    //   NVIC_EnableIRQ(UART3_IRQn);
    //   break;
  }
  uart_clk = SystemCoreClock/(2);
  baud_icoef = uart_clk / (16 * baudrate);
  baud_fcoef = ((uart_clk / (16.0f * baudrate) - baud_icoef) * 64 + 0.5f);
  pUART_ref->IBRD = baud_icoef;
  pUART_ref->FBRD = baud_fcoef;
  pUART_ref->LCRH = (3<<5);  // 8-bit, parity - disable
  /*interrupt*/
  pUART_ref->IMSC |= UART_IMSC_RXIM_Msk;
  /*timer*/
  if((TMR3->CTRL & TMR_CTRL_ON_Msk) == 0) {
    RCU->PCLKCFG0 |= RCU_PCLKCFG0_TMR3EN_Msk;
    RCU->PRSTCFG0 |= RCU_PRSTCFG0_TMR3EN_Msk;
    TMR3->LOAD = ((uint64_t)(SystemCoreClock / 2) * TIMER_LOAD_US) / 1000000;
    TMR3->CTRL = TMR_CTRL_INTEN_Msk | TMR_CTRL_ON_Msk;
    NVIC_EnableIRQ(TMR3_IRQn);
    }
  pUART_ref->CR |= UART_CR_RXE_Msk | UART_CR_TXE_Msk | UART_CR_UARTEN_Msk;
}



int UART_Rx(UART_TypeDef *pUART_ref, uint8_t *bt) {
  int ret;
  int uart_num;
  uart_num = uart_get_num(pUART_ref);
  if(UARTRxBuffHead[uart_num] == UARTRxBuffTail[uart_num]) {
    ret = 0;
    }
  else {
    *bt = UARTRxBuff[uart_num][UARTRxBuffTail[uart_num]++];
    UARTRxBuffTail[uart_num] &= (UART_RX_BUFF_SIZE - 1);
    ret = 1;
    }
  return ret;
}

int UART_RxPacket(UART_TypeDef *pUART_ref, uint8_t *buff, uint16_t buff_size) {
  int uart_num, i;
  uint16_t rx_size, tot_size = 0;
  uart_num = uart_get_num(pUART_ref);
  tot_size = 0;
  if(FrameGapRx[uart_num] == 0) {
    rx_size = (UARTRxBuffHead[uart_num] - UARTRxBuffTail[uart_num]) & (UART_RX_BUFF_SIZE - 1);
    if(rx_size) {
      for(i=0; i<rx_size; i++) {
        if(tot_size >= buff_size)  break;
        *buff++ = UARTRxBuff[uart_num][UARTRxBuffTail[uart_num]++];
        UARTRxBuffTail[uart_num] &= (UART_RX_BUFF_SIZE - 1);
        tot_size++;        
        }
      }
    }
  return tot_size;
}

/**
 * @brief ��������� ����� ��������� ������ � ����������� � �����
 * 
 * @param pUART_ref 
 * @param buff 
 * @param rx_len 
 * @return in8_t 
 */
int8_t UART_RxFullPacket(UART_TypeDef *pUART_ref, uint8_t *buff, uint16_t* rx_len) 
{
  int uart_num, i;
  uint16_t rx_size = 0, tot_size = 0;
  uart_num = uart_get_num(pUART_ref);
  if(FrameGapRx[uart_num] == 0) {
    rx_size = (UARTRxBuffHead[uart_num] - UARTRxBuffTail[uart_num]) & (UART_RX_BUFF_SIZE - 1);
    if(rx_size) {
      for(i=0; i<rx_size; i++) {
        if(tot_size >= rx_size)  break;
        *buff++ = UARTRxBuff[uart_num][UARTRxBuffTail[uart_num]++];
        UARTRxBuffTail[uart_num] &= (UART_RX_BUFF_SIZE - 1);
        tot_size++;        
      }
      *rx_len = tot_size;
      return 1;
    }
  }
  else{
    return 0;
  }
	return 0;
}

void UART_Tx(UART_TypeDef *pUART_ref, uint8_t bt) {
  int uart_num;
  uart_num = uart_get_num(pUART_ref);
  UARTTxBuff[uart_num][UARTTxBuffHead[uart_num]++] = bt;
  UARTTxBuffHead[uart_num] &= (UART_TX_BUFF_SIZE - 1);
  uart_tx_enable(pUART_ref, 1);
  UARTForceTxIrq = 1;
  uart_IRQHandler(pUART_ref);
}

void UART_TxPacket(UART_TypeDef *pUART_ref, uint8_t *buff, uint16_t leng) {
  int uart_num;
  int i;
  uart_num = uart_get_num(pUART_ref);
  while(FrameGapTx[uart_num]);  // frame gap wait
  for(i=0; i<leng; i++) {
    UARTTxBuff[uart_num][UARTTxBuffHead[uart_num]++] = buff[i];
    UARTTxBuffHead[uart_num] &= (UART_TX_BUFF_SIZE - 1);
    }
  if(leng != 0) {
    uart_tx_enable(pUART_ref, 1);
    UARTForceTxIrq = 1;
    uart_IRQHandler(pUART_ref);
    }
}

/**
 * @brief �������� �� ������� �������� ������ ������.
 * 
 * @param pUART_ref 
 * @return int8_t 1 - ���� �����, 0 - ��� ������
 */
int8_t UART_PacketInWaiting(UART_TypeDef *pUART_ref)
{
  int uart_num;
  uart_num = uart_get_num(pUART_ref);
  if(FrameGapRx[uart_num] != 0){
    return 1;
  }
  else{
    return 0;
  }
}

/**
 * @brief �������� �� ���������� ������
 * 
 * @param pUART_ref 
 * @return int8_t 1- ����� �����, 0 - ����� �� �����
 */
int8_t UART_PacketReady(UART_TypeDef *pUART_ref)
{
  int uart_num;
  uint16_t rx_size;
  uart_num = uart_get_num(pUART_ref);
  rx_size = (UARTRxBuffHead[uart_num] - UARTRxBuffTail[uart_num]) & (UART_RX_BUFF_SIZE - 1);
  if ((FrameGapRx[uart_num] == 0) && (rx_size)){
    return 1;
  }
  else{
    return 0;
  }
}

/*----------------------------------------------------------------------------------------------*/

void uart_IRQHandler(UART_TypeDef *pUART_ref) {
  int uart_num;
  uint32_t ris;
  uart_num = uart_get_num(pUART_ref);
  ris = pUART_ref->RIS;
  if(( UARTTxCompleteFlg[uart_num] ) && (ris & UART_RIS_TDRIS_Msk )) {  //Tx complete
    uart_tx_enable(pUART_ref, 0);
    pUART_ref->ICR = UART_ICR_TDIC_Msk;
    pUART_ref->IMSC &= ~UART_IMSC_TDIM_Msk;
    UARTTxCompleteFlg[uart_num] = 0;
    FrameGapTx[uart_num] = FRAME_GAP_TX_CNT;
    }
  else {
    if(ris & UART_RIS_RXRIS_Msk ) {  // RX interrupt
      if(FrameGapRx[uart_num] == 0) {
        UARTRxBuffHead[uart_num] = 0;
        UARTRxBuffTail[uart_num] = 0;
      }
      UARTRxBuff[uart_num][UARTRxBuffHead[uart_num]++] = (uint8_t)pUART_ref->DR;
      UARTRxBuffHead[uart_num] &= (UART_RX_BUFF_SIZE - 1);
      FrameGapRx[uart_num] = FRAME_GAP_RX_CNT;
    }  
    else if((ris & UART_RIS_TXRIS_Msk)||(UARTForceTxIrq)) {  // TX interrupt
      pUART_ref->ICR = UART_ICR_TXIC_Msk;
      if(UARTTxBuffHead[uart_num] != UARTTxBuffTail[uart_num]) {
        pUART_ref->DR = UARTTxBuff[uart_num][UARTTxBuffTail[uart_num]++];
        UARTTxBuffTail[uart_num] &= (UART_TX_BUFF_SIZE - 1);
      }
      if(UARTTxBuffHead[uart_num] == UARTTxBuffTail[uart_num]) {
        pUART_ref->IMSC &= ~UART_IMSC_TXIM_Msk;
        pUART_ref->IMSC |= UART_IMSC_TDIM_Msk;
        UARTTxCompleteFlg[uart_num] = 1;
      }
      if(UARTForceTxIrq) {
        UARTForceTxIrq = 0;        
        pUART_ref->IMSC |= UART_IMSC_TXIM_Msk ;
      }
      FrameGapTx[uart_num] = FRAME_GAP_TX_CNT;
    }
  }
  __DSB();__ISB();
}

void UART0_IRQHandler() {
  uart_IRQHandler(UART0);
}
void UART1_IRQHandler() {
  uart_IRQHandler(UART1);
}
void UART2_IRQHandler() {
  uart_IRQHandler(UART2);
}
void UART3_IRQHandler() {
  uart_IRQHandler(UART3);
}

#pragma O2

void TMR3_IRQHandler() {
  int i;
   TMR3->INTSTATUS = TMR_INTSTATUS_INT_Msk;
  for(i=0; i<4; i++) {
    if(FrameGapRx[i]) FrameGapRx[i]--;
    if(FrameGapTx[i]) FrameGapTx[i]--;
    }
}
