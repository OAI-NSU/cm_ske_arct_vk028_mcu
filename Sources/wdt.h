#ifndef _WDT_H_
#define _WDT_H_

#include "main.h"
#include "CM4/K1921VK028.h"
#include "system_K1921VK028.h"

#define WD_TIME_1S (2000000)
// Интервал сторожевого таймера 3 sec
#define WD_TIME    (WD_TIME_1S*3)
//! Для обратной совместимости
#define WDRST WDT_Reset()


void WDT_Init(void);
void WDT_Reset(void);


#endif
