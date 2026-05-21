  /**
  ******************************************************************************
  * @file           : clock.c
  * @version        : v1.0
  * @brief          : надстройка над HAL для работы с RTC
  * @author			: Стюф Алексей/Alexey Styuf <a-styuf@yandex.ru>
  ******************************************************************************
  */

#include "clock.h"

char clock_str[32] = {0};

/**
  * @brief  устанавливаем время в ноль при перезагрузке
  */
void clock_init(uint32_t init_time_s)
{
  // включение тактирования модуля
  RCU->PCLKCFG0 |= RCU_PRSTCFG0_RTCEN_Msk;
  RCU->PRSTCFG0 |= RCU_PRSTCFG0_RTCEN_Msk;
  //
  clock_set_time_s(init_time_s);
}

/**
  * @brief  установка времени в RTC
  * @param  time_s: время в секундах от 2000 года
  */
void clock_set_time_s(uint32_t time_s)
{
  uint32_t years = 0, days = 0, days_remain = 0, month = 0;
  uint16_t years_d_offset[100] = {0};
  //
  days = time_s / 86400;
  //
  for(uint8_t y=0; y<100; y++){
      years_d_offset[y] = y*365 + (y+3)/4 - (y+99)/100 + (y+399)/400;
      if (y > 0){
          if (days < years_d_offset[y]){
              years = (y-1);
              days_remain = days - years_d_offset[y-1];
              break;
          }
      }
  }
  //
  for(uint8_t m=1; m<=12; m++){
    if (days_remain < _get_month_offset(years, m)){
        month = m-1;
        days_remain = days_remain - _get_month_offset(years, m-1);
        break;
    }
  }
  //
	//printf("%d-%d-%d %d:%d:%d d_r:%d", years, month, days_remain+1, (time_s % 86400) / 3600, ((time_s % 86400) % 3600) / 60, ((time_s % 86400) % 3600) % 60, days_remain);
	//
  RTC->YEAR = dec2bcd(years%100);
  RTC->MONTH = dec2bcd(month);
  RTC->DAY = dec2bcd(days_remain + 1);
  RTC->DOW = 1; //TODO: заглушка
  //
  RTC->HOUR = dec2bcd((time_s % 86400) / 3600);
  RTC->MIN =  dec2bcd(((time_s % 86400) % 3600) / 60);
  RTC->SEC =  dec2bcd(((time_s % 86400) % 3600) % 60);
  //
  RTC->POS =  0;
}

/**
  * @brief  установка времени в HAL_RTC c мс
  * @param  time_s: время в секундах от 2000 года
  */
void clock_set_time_s_and_ms(uint32_t time_s, uint16_t ms)
{
  clock_set_time_s(time_s);
  //
  RTC->POS =  (uint16_t)((ms%1000)*1024/1000.);
}

/**
  * @brief  установка времени в RTC со взятием разницы устанавливаемого времени и текущего
  * @param  time_s: время в секундах от 2000 года
  */
void clock_set_time_s_with_diff(uint32_t time_s, int16_t* diff_time_s)
{
  *diff_time_s = time_s - clock_get_time_s();
  clock_set_time_s(time_s);
}

/**
  * @brief  получение количества секунд с 2000-года
  */
uint32_t clock_get_time_s(void)
{
  uint32_t seconds, days, d_years, d_month;
  uint8_t y, m, d, H, M, S;
  //
	RTC->SHDW &= ~RTC_SHDW_UPDTEN_Msk;
  //
  y = bcd2dec(RTC->YEAR);
  m = bcd2dec(RTC->MONTH);
  d = bcd2dec(RTC->DAY);
  H = bcd2dec(RTC->HOUR);
  M = bcd2dec(RTC->MIN);
  S = bcd2dec(RTC->SEC);


  d_years = y*365 + (y+3)/4 - (y+99)/100 + (y+399)/400;
  d_month = _get_month_offset(y, m);
  days = (d) + (d_month-1) + d_years;
  //
  seconds = days*86400 + H*3600 + M*60 + S;
  //
  // printf("%d-%d-%d %d:%d:%d, days %d", y, m, d, (seconds % 86400) / 3600, ((seconds % 86400) % 3600) / 60, ((seconds % 86400) % 3600) % 60, days);
  //
	RTC->SHDW |= RTC_SHDW_UPDTEN_Msk;
	return seconds;
}

/**
  * @brief  получение массива данных в формате hex2dec YYMMDD-hhmmss
  */
void clock_get_bcd_array(uint8_t *array)
{
  //
	RTC->SHDW &= ~RTC_SHDW_UPDTEN_Msk;
  //
  array[0] = RTC->YEAR;
  array[1] = RTC->MONTH;
  array[2] = RTC->DAY;
  array[3] = RTC->HOUR;
  array[4] = RTC->MIN;
  array[5] = RTC->SEC;
  //
	RTC->SHDW |= RTC_SHDW_UPDTEN_Msk;
}

/**
  * @brief  получение массива данных в формате hex2dec YYMMDD-hhmmss
  */
void clock_get_bcd_array_with_fractional_part(uint8_t *array)
{
  //
	RTC->SHDW &= ~RTC_SHDW_UPDTEN_Msk;
  //
  array[0] = RTC->YEAR;
  array[1] = RTC->MONTH;
  array[2] = RTC->DAY;
  array[3] = RTC->HOUR;
  array[4] = RTC->MIN;
  array[5] = RTC->SEC;
  array[6] = dec2bcd(((uint16_t)(RTC->POS*1000/1024.)/100) & 0xFF);
  array[7] = dec2bcd(((uint16_t)(RTC->POS*1000/1024.)%100) & 0xFF);
  //
	RTC->SHDW |= RTC_SHDW_UPDTEN_Msk;
}

/**
  * @brief  получение массива данных в формате hex2dec YYMMDD-hhmmss
  */
char* clock_get_str(void)
{
  uint8_t clock_array[8] = {0};
  clock_get_bcd_array_with_fractional_part(clock_array);
  sprintf(clock_str, "%02X-%02X-%02X %02X:%02X:%02X.%01X%02X",  clock_array[0], clock_array[1], clock_array[2], clock_array[3], 
                                                    clock_array[4], clock_array[5], clock_array[6], clock_array[7]);
  return clock_str;
}


/**
  * @brief  получение массива данных в формате hex2dec YYMMDD-hhmmss
  */
volatile char* now(void)
{
  return clock_get_str();
}

uint16_t _get_month_offset(uint8_t year, uint8_t month)
{
    const uint16_t month_d_offset[12] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
    uint8_t month_d_add = ((year % 4 == 0) && ((year % 100 != 0) && (year % 400 != 0)) && (month>2)) ? 1 : 0;
    return month_d_offset[month-1] + month_d_add;
}

/* Standard iterative function to convert 16-bit integer to BCD */
uint8_t bcd2dec(uint8_t val)
{
  return ( (val/16*10) + (val%16) );
}

uint8_t dec2bcd(uint8_t val)
{
  return ( (val/10*16) + (val%10) );
}
