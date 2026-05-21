#ifndef _timer_H_
#define _timer_H_

#include "main.h"

#pragma pack(push, 2)

/**
  * @brief  структура для удобства работы с временем в 1/256 секунд
*/
typedef struct
{
    uint8_t low_part;
    uint32_t mid_part;
    uint8_t high_part;
    uint16_t zero_part;
}typeCMTime;

#pragma pack(pop)

void Timers_Init(void);
void Timers_Start(uint8_t num, uint32_t time_ms);
void Timers_Stop(uint8_t num);
uint8_t Timers_Status(uint8_t num);
void Timer_Delay(uint8_t num, uint32_t delay_ms);

__weak void TimerIRQ_CallBack(TMR_TypeDef* tmr_ptr);
#endif
