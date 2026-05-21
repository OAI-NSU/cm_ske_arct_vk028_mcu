#ifndef _TERMO_RES_H_
#define _TERMO_RES_H_

#include <string.h>
#include <stdint.h>
#include "main.h"

#define TRES_V_REF    (5.0)
#define TRES_R1_REF   (1E3) // 1k Ohm - нижнее верхнего резистора

#define TRES_ADC_A (3.3/4096.0)  // коэффициенты пересчета значений АЦП по формуле U(V) = A*ADC + B
#define TRES_ADC_B (0)  

#define TRES_TYPE     (1E3) // Pt1000

#define TRES_CAL_TEMP   {-100,  -70,   -50,   -40,   -30,   -20,   -10,   +0,     +10,    +20,    +30,    +40,    +50,    +70,    +100,   +200}
#define TRES_CAL_RES    {602.6, 723.3, 803.1, 842.7, 882.2, 921.6, 960.9, 1000.0, 1039.0, 1077.9, 1116.7, 1155.4, 1194.0, 1270.7, 1385.0, 1758.4}

#pragma pack(push, 2)

/** 
  * @brief  структура управления отдельным термо-резистором, включенным по схеме резистивного делителя (R1 - верхнее плечо, термосопротивление - нижнее плечо)
  */
typedef struct
{
  float v_ref, v_out;
  float r1_val, tres_val;
  float temp;
  uint16_t temp_u16;
} type_TRES_model;

#pragma pack(pop)

//
void tres_init(type_TRES_model* t_res_ptr);
void tres_set_parameters(type_TRES_model* t_res_ptr, float vref, float r1_val, float tres_val);
float tres_get_temp(type_TRES_model* t_res_ptr);
uint16_t tres_get_temp_u16(type_TRES_model* t_res_ptr);
void tres_adc_data_process(type_TRES_model* t_res_ptr, uint16_t adc_data);

float _linear_interpolation(float x, float* array_y, float* array_x, uint16_t length);
float _calc_tr_res(float u_ref, float u_sign, float r_1);

#endif
