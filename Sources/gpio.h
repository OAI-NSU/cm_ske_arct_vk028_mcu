#ifndef _GPIO_H_
#define _GPIO_H_

#include "main.h"
#include "gpio_conf.h"

#pragma pack(push, 2)

/** 
  * @brief  структура управления одним GPIO
  */
typedef struct
{
  GPIO_TypeDef* port;
  uint8_t line;
} type_SINGLE_GPIO;

/** 
  * @brief  структура для быстрого поиска источника прерываний
  * позволяет не просматривать все gpio для определения порядкового номера, а пробежать только те, у которых разрешено прерывание
  */
typedef struct
{
  uint8_t flag;  //! способ определения наличия информации внутри отдельного элемента массива
  uint8_t io_num;
  GPIO_TypeDef* port;
  uint8_t line;
} type_GPIO_IRQ_Info;

/** 
  * @brief  структура управления одним GPIO
  */
typedef struct
{
  type_SINGLE_GPIO io[OUT_NUM];
  type_SINGLE_GPIO stm[STM_IO_NUM];
  // сопроводительная информация
  type_GPIO_IRQ_Info io_irq_info[OUT_NUM]; //! создаем избыточное количество элементов для гарантии непереполнения массива
  uint8_t irq_info_num;
} type_GPIO_OAI_cm;

#pragma pack(pop)

//
void oai_cm_gpio_init(type_GPIO_OAI_cm* oai_io_ptr);
int8_t oai_cm_io_find(type_GPIO_OAI_cm* oai_io_ptr, GPIO_TypeDef *port, uint8_t line);
// управление отдельным GPIO
void gpio_init(type_SINGLE_GPIO *gpio_ptr, GPIO_TypeDef* port, uint8_t line, uint8_t type, uint8_t irq, uint8_t state);
void gpio_set(type_SINGLE_GPIO* gpio_ptr, uint8_t val);
uint8_t gpio_get(type_SINGLE_GPIO* gpio_ptr);

__weak void INT_PORT_CallBack(GPIO_TypeDef *port, uint8_t line);
void INT_PORT_Handler(GPIO_TypeDef *port);

#endif
