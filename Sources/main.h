/**
 * @file main.h
 * @author Alexey Styuf (a-styuf@yandex.ru)
 * @brief Peripheral settings and include files, that will be used in program
 * @version 0.1
 * @date 2024-02-04
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#ifndef _MAIN_H_
#define _MAIN_H_

#include "CM4/K1921VK028.h"
#include "system_K1921VK028.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "wdt.h"

#define DEBUG

// #define SystemCoreClock 100000000

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

#endif
