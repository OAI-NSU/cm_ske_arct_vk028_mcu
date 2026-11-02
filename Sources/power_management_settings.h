
/**
 * @brief Настройки количества и типов каналов питания, а также обвязки измерения тока
 * 
 */

#ifndef _POWER_MANAGER_SETTINGS_H_
#define _POWER_MANAGER_SETTINGS_H_


#include "main.h"
#include "pwr_channel.h"

/**
  * @brief  определяем номера каналов, последнее поле - количество каналов
*/
typedef enum PWR_CH
{
  PWR_BE,       PWR_CM,       PWR_MPP1,     PWR_MPP2,
  PWR_MPP3,     PWR_MPP4,     PWR_DEP_AD1,  PWR_DEP_AD2,
  PWR_DEP_AD3,  PWR_DEP_AD4,  PWR_DEP_AD5,  PWR_DEP_AD6, 
  PWR_DEP_MKKD,  
  PWR_CH_NUMBER
} type_PWR_CH;

/**
  * @brief  распределение каналов в соответствии с enum PWR_CH
  * @note   длина совпадает с PWR_CH_NUMBER
  */
#define PWR_CAL_RES_SHUNT_OHM   { 2.5E-1f, 1.0E0f,    1.0E0f, 1.0E0f, 1.0E0f, 1.0E0f,   1.0E0f, 1.0E0f, 1.0E0f, 1.0E0f, 1.0E0f, 1.0E0f,  1.0E0f}
#define PWR_CAL_FB_SHUNT_OHM    { 5.1E+3f, 1.0E4f,    6.8E3f, 6.8E3f, 6.8E3f, 6.8E3f,   5.1E3f, 5.1E3f, 5.1E3f, 5.1E3f, 5.1E3f, 5.1E3f,  5.1E3f}

#define PWR_CURRENT_BOUND       { 3*815,   3*45,      3*70,   3*70,   3*70,   3*70,     3*70,   3*70,   3*70,   3*70,   3*70,   3*70,   3*70} // граница тока в мА для каждого канала

#define PWR_CHANNELS_TYPES      { PWR_CH_CTRL_NU, PWR_CH_CTRL_NU, PWR_CH_FLAG, PWR_CH_FLAG, \
                                  PWR_CH_FLAG,    PWR_CH_FLAG,    PWR_CH_DV_CTRL, PWR_CH_DV_CTRL, \
                                  PWR_CH_DV_CTRL, PWR_CH_DV_CTRL, PWR_CH_DV_CTRL, PWR_CH_DV_CTRL, \
                                  PWR_CH_DV_CTRL }

#define PWR_AUTO_CTRL           { 0, 0,  1, 1, 1, 1,  1, 1, 1, 1, 1, 1,  1 }
#define PWR_DOUBLE_OUT          { 0, 0,  0, 0, 0, 0,  1, 1, 1, 1, 1, 1,  1 }

#define PWR_DEFAULT_STATE       { PWR_CH_ON,  PWR_CH_ON,  PWR_CH_ON, PWR_CH_ON, \
                                  PWR_CH_OFF, PWR_CH_OFF, PWR_CH_ON, PWR_CH_ON, \
                                  PWR_CH_ON,  PWR_CH_ON,  PWR_CH_ON, PWR_CH_ON, \
                                  PWR_CH_ON }

#define PWR_DEFAULT_HALF_SET    { 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0, 0,  1 }
#define PWR_INIT_TIMEOUT_MS     ( 10000 )
#define PWR_DEFAULT_DELAY       { 0,      0,      1*300,  2*300, \
                                  3*300,  4*300,  6*300,  7*300, \
                                  8*300,  9*300,  10*300, 11*300,\
                                  13*300 }

// Конфигурация GPIO - это массив из 5-ти GPIO: 
// PWR_CH_CTRL_NU - не используются
// PWR_CH_FLAG - используются 2 GPIO: ena и inh
// PWR_CH_PULSE - используются 2 GPIO: on и off
// PWR_CH_DV_CTRL - используются 5 GPIO: A, B, C, D, E

#define PWR_GPIO_PORT_CFG { \
                            {OUT_NU, OUT_NU, OUT_NU, OUT_NU, OUT_NU}, \
                            {OUT_NU, OUT_NU, OUT_NU, OUT_NU, OUT_NU}, \
                            \
                            {OUT_NU, OUT_1, OUT_NU, OUT_NU, OUT_NU},  \
                            {OUT_NU, OUT_0, OUT_NU, OUT_NU, OUT_NU},  \
                            {OUT_NU, OUT_3, OUT_NU, OUT_NU, OUT_NU},  \
                            {OUT_NU, OUT_2, OUT_NU, OUT_NU, OUT_NU},  \
                            \
                            {OUT_15, OUT_13, OUT_12, OUT_11, OUT_4},  \
                            {OUT_15, OUT_13, OUT_12, OUT_11, OUT_5},  \
                            {OUT_15, OUT_13, OUT_12, OUT_11, OUT_6},  \
                            {OUT_15, OUT_13, OUT_12, OUT_11, OUT_7},  \
                            {OUT_15, OUT_13, OUT_12, OUT_11, OUT_8},  \
                            {OUT_15, OUT_13, OUT_12, OUT_11, OUT_9},  \
                            \
                            {OUT_15, OUT_13, OUT_12, OUT_11, OUT_10}   \
                          }

#define PWR_ADC_CFG       { \
                            {1}, \
                            {18},\
                            \
                            {0}, \
                            {3}, \
                            {2}, \
                            {5}, \
                            \
                            {4}, \
                            {7}, \
                            {6}, \
                            {9}, \
                            {8}, \
                            {11}, \
                            \
                            {10} \
                          }

#endif
