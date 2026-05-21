/**
 * @file timers.c
 * @author a-styuf (a-styuf@yandex.ru)
 * @brief описание аппаратной реализации таймеров для 1986ВЕ8, минимальное количество таймеров - 3
 *< TMR0 - для общего использования
 *< TMR1 - для общего использования
 *< TMR3 - используются для работы внутренней шины (здесь не представлен)
 * Таймеры используют прерывания для выставления статусов.
 * @version 0.1
 * @date 2024-02-06
 * 
 * @copyright Copyright (c) 2024
 * 
 */


#include "timers.h"

volatile uint8_t time_out = 0;  //! глобальная переменная для реализации отложенных вызовов
uint8_t high_byte_time = 0;  //! глобальная переменная для хранения старшей части глобального времени

/**
 * @brief инициализация таймеров для использования в программе
 * TMR0 - для общего использования
 * TMR1 - для общего использования
 * TMR3 - используются для работы внутренней шины (здесь не представлен)
 */
void Timers_Init(void)
{   
    //TMR0
    RCU->PCLKCFG0 |= RCU_PCLKCFG0_TMR0EN_Msk;
    RCU->PRSTCFG0 |= RCU_PRSTCFG0_TMR0EN_Msk;
    TMR0->LOAD = ((uint64_t)(SystemCoreClock / 2) * 1) / 1000; //1 мс
    TMR0->CTRL = TMR_CTRL_INTEN_Msk | TMR_CTRL_ON_Msk;
    NVIC_EnableIRQ(TMR0_IRQn);
    //TMR1
    RCU->PCLKCFG0 |= RCU_PCLKCFG0_TMR1EN_Msk;
    RCU->PRSTCFG0 |= RCU_PRSTCFG0_TMR1EN_Msk;
    TMR1->LOAD = ((uint64_t)(SystemCoreClock / 2) * 1) / 1000; //1 мс
    TMR1->CTRL = TMR_CTRL_INTEN_Msk | TMR_CTRL_ON_Msk;
    NVIC_EnableIRQ(TMR1_IRQn);
}

/**
 * @brief Работа с заданием задержек. Универсально для TMR0, TMR1
 * 
 * @param num 0 - TMR0, 1 - TMR1
 * @param time_ms задержка в мс
 */
void Timers_Start(uint8_t num, uint32_t time_ms)
{  
    switch (num)
    {
        case 0:
            NVIC_DisableIRQ(TMR0_IRQn);
            TMR0->LOAD = 0;
            TMR0->VALUE = ((uint64_t)(SystemCoreClock / 2) * time_ms) / 1000;
            TMR0->CTRL = TMR_CTRL_INTEN_Msk | TMR_CTRL_ON_Msk;
            NVIC_EnableIRQ(TMR0_IRQn);
            break;
        case 1:
            NVIC_DisableIRQ(TMR1_IRQn);
            TMR1->LOAD = 0;
            TMR1->VALUE = ((uint64_t)(SystemCoreClock / 2) * time_ms) / 1000;
            TMR1->CTRL = TMR_CTRL_INTEN_Msk | TMR_CTRL_ON_Msk;
            NVIC_EnableIRQ(TMR1_IRQn);
            break;
    }
}

/**
 * @brief Остановка работы таймера по номеру.
 * 
 * @param num  0 - TMR0, 1 - TMR1
 */
void Timers_Stop(uint8_t num)
{  
    switch (num)
    {
        case 0:
            NVIC_DisableIRQ(TMR0_IRQn);
            TMR0->CTRL = 0;
            TMR0->VALUE = 0;
            TMR0->LOAD = 0;
            NVIC_EnableIRQ(TMR0_IRQn);
            break;
        case 1:
            NVIC_DisableIRQ(TMR1_IRQn);
            TMR1->CTRL = 0;
            TMR1->VALUE = 0;
            TMR1->LOAD = 0;
            NVIC_EnableIRQ(TMR1_IRQn);
            break;
    }
}

/**
 * @brief проверка работы таймера
 * 
 * @param num 0 - TMR0, 1 - TMR1
 * @return uint8_t 1 - таймер еще считает; 0-закончил счет
 */
uint8_t Timers_Status(uint8_t num)
{
    switch(num){
        case 0:
            if (TMR0->VALUE == 0){
                return 0;
            }
            else{
                return 1;
            }
        case 1:
            if (TMR1->VALUE == 0){
                return 0;
            }
            else{
                return 1;
            }
    }
    return 0;
}


/**
  * @brief  запуск блокирующей задержки
  * @param  num номер таймера
  * @param  delay_ms  величина задержки в мс
  */
void Timer_Delay(uint8_t num, uint32_t delay_ms)
{
    Timers_Start(num, delay_ms); 
    while (Timers_Status(num) & 1);
}

void TMR0_IRQHandler(void) 
{
    NVIC_DisableIRQ(TMR0_IRQn);
    TMR0->LOAD = 0;
    TMR0->VALUE = 0;
    TMR0->CTRL = 0;
    TMR0->INTSTATUS = 1;
    //
    TimerIRQ_CallBack(TMR0);
    //
    NVIC_EnableIRQ(TMR0_IRQn);
}

void TMR1_IRQHandler(void) 
{
    NVIC_DisableIRQ(TMR1_IRQn);
    TMR1->LOAD = 0;
    TMR1->VALUE = 0;
    TMR1->CTRL = 0;
    TMR1->INTSTATUS = 1;
    //
    TimerIRQ_CallBack(TMR1);
    //
    NVIC_EnableIRQ(TMR1_IRQn);
}

__weak void TimerIRQ_CallBack(TMR_TypeDef* tmr_ptr)
{
    //
}
