#ifndef _CLOCK_H
#define _CLOCK_H

#include <stdio.h>
#include "CM4/K1921VK028.h"
#include "system_K1921VK028.h"

#ifndef int32_t
    /* exact-width signed integer types */
    typedef   signed          char int8_t;
    typedef   signed short     int int16_t;
    typedef   signed           int int32_t;

        /* exact-width unsigned integer types */
    typedef unsigned          char uint8_t;
    typedef unsigned short     int uint16_t;
    typedef unsigned           int uint32_t;
#endif

void clock_init(uint32_t init_time_s);
void clock_set_time_s(uint32_t time_s);
void clock_set_time_s_and_ms(uint32_t time_s, uint16_t ms);
void clock_set_time_s_with_diff(uint32_t time_s, int16_t* diff_time_s);
uint32_t clock_get_time_s(void);
void clock_get_bcd_array(uint8_t *array);
void clock_get_bcd_array_with_fractional_part(uint8_t *array);
char* clock_get_str(void);
volatile char* now(void);

uint16_t _get_month_offset(uint8_t year, uint8_t month);
uint8_t bcd2dec(uint8_t val);
uint8_t dec2bcd(uint8_t val);

#endif
