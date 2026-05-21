  /**
  ******************************************************************************
  * @file           : io.c
  * @version        : v1.0
  * @brief          : функции для работы с GPIO. Явный недостаток - невозможность управления одновременно группой io.
  * @author			    : Стюф Алексей/Alexe Styuf <a-styuf@yandex.ru>
  ******************************************************************************
  */

#include "gpio.h"

/**
 * @brief инициализация всех доступных дискретных сигналов на плате OAI_CM
 * 
 * @param gpio_ptr 
 */
void oai_cm_gpio_init(type_GPIO_OAI_cm* oai_io_ptr)
{
  RCU->HRSTCFG |= RCU_HRSTCFG_GPIOBEN_Msk;  RCU->HCLKCFG |= RCU_HCLKCFG_GPIOBEN_Msk;
  RCU->HRSTCFG |= RCU_HRSTCFG_GPIOCEN_Msk;  RCU->HCLKCFG |= RCU_HCLKCFG_GPIOCEN_Msk;
  RCU->HRSTCFG |= RCU_HRSTCFG_GPIOMEN_Msk;  RCU->HCLKCFG |= RCU_HCLKCFG_GPIOMEN_Msk;
  RCU->HRSTCFG |= RCU_HRSTCFG_GPIOLEN_Msk;  RCU->HCLKCFG |= RCU_HCLKCFG_GPIOLEN_Msk;
  //
	NVIC_DisableIRQ(GPIOL_IRQn);
  NVIC_DisableIRQ(GPIOM_IRQn);
	//
  GPIO_TypeDef* io_port[OUT_NUM] = GPIO_PORT;
  GPIO_TypeDef* stm_port[STM_IO_NUM] = STM_PORT;
  //
  uint8_t io_line[OUT_NUM] = GPIO_LINE;
  uint8_t stm_line[STM_IO_NUM] = STM_LINE;
  //
  uint8_t io_type[OUT_NUM] = GPIO_IN_OUT;
  uint8_t stm_type[STM_IO_NUM] = STM_IN_OUT;
  //
  uint8_t io_irq[OUT_NUM] = GPIO_IRG_SET;
  uint8_t stm_irq[STM_IO_NUM] = STM_IRG_SET;
  //
  uint8_t io_def_state[OUT_NUM] = GPIO_DEF_STATE;
  uint8_t stm_def_state[STM_IO_NUM] = STM_DEF_STATE;
  //
  memset(oai_io_ptr->io_irq_info, 0x00, sizeof(oai_io_ptr->io_irq_info));
  oai_io_ptr->irq_info_num = 0;
  //
  for (uint8_t num = 0; num<OUT_NUM; num++){
    gpio_init(&oai_io_ptr->io[num], io_port[num], io_line[num], io_type[num], io_irq[num], io_def_state[num]);
    // заполнение дополнительной информации для быстрого поиска по прерываниям
    if((io_irq[num] == GPIO_IRQ_ON) && (io_type[num] == IN)){
      //
      oai_io_ptr->io_irq_info[oai_io_ptr->irq_info_num].flag = 1;
      oai_io_ptr->io_irq_info[oai_io_ptr->irq_info_num].io_num = num;
      oai_io_ptr->io_irq_info[oai_io_ptr->irq_info_num].port = io_port[num];
      oai_io_ptr->io_irq_info[oai_io_ptr->irq_info_num].line = io_line[num];
      //
      oai_io_ptr->irq_info_num++; 
    }
  }
  for (uint8_t num = 0; num<STM_IO_NUM; num++){
    gpio_init(&oai_io_ptr->stm[num], stm_port[num], stm_line[num], stm_type[num], stm_irq[num], stm_def_state[num]);
  }
  //
  // включение прерываний только по их наличию
  if(oai_io_ptr->irq_info_num){
    NVIC_EnableIRQ(GPIOL_IRQn);
    NVIC_EnableIRQ(GPIOM_IRQn);
  }
}

/**
 * @brief поиск источника прерывания по порту и номеру линии
 * в случае отсутствия подобного вывод возвращает -1
 * 
 * @param port 
 * @param line 
 * @return int8_t -1 - порт не найден, другое - ок
 */
int8_t oai_cm_io_find(type_GPIO_OAI_cm* oai_io_ptr, GPIO_TypeDef *port, uint8_t line)
{
  for(uint8_t num=0; num<OUT_NUM; num++){
    if((oai_io_ptr->io_irq_info[num].flag == 0)){
      return -1;
    }
    else if((oai_io_ptr->io_irq_info[num].port == port) && (oai_io_ptr->io_irq_info[num].line == line)) {
      return oai_io_ptr->io_irq_info[num].io_num;
    }
  }
  return -1;
}

/**
 * @brief инициализация отдельного  GPIO
 * 
 * @param gpio_ptr указатель на единичный GPIO
 * @param port указатель на структуру управления портом
 * @param line номер линии
 * @param type тип: вход/выход
 * @param irq необходимость прерывания
 * @param state начальное состояние
 */
void gpio_init(type_SINGLE_GPIO *gpio_ptr, GPIO_TypeDef* port, uint8_t line, uint8_t type, uint8_t irq, uint8_t state)
{
  gpio_ptr->port = port;
  gpio_ptr->line = line;
  // включение цифрового пина
  gpio_ptr->port->DENSET = (1<<(gpio_ptr->line));
  // переделка пина в выход
  if ((type == OUT) || (type == OPEN_DRAIN)) gpio_ptr->port->OUTENSET = (1<<(gpio_ptr->line));
  if (type == OPEN_DRAIN) gpio_ptr->port->OUTMODE |= (1<<(gpio_ptr->line*2));
  // установка подтяжки
  if ((type == IN) || (type == OPEN_DRAIN)) gpio_ptr->port->PULLMODE |= ((0x01)<<(2*gpio_ptr->line));
  // установка состояния по умолчанию
  if ((type == OUT) || (type == OPEN_DRAIN)) gpio_ptr->port->DATAOUTSET = ((state & 0x01)<<(gpio_ptr->line));
  // инициализация работы с прерываниями
  if ((irq == GPIO_IRQ_ON) && (type == IN)) {
    gpio_ptr->port->INTENSET = (1<<(gpio_ptr->line));
    gpio_ptr->port->INTTYPESET = (1<<(gpio_ptr->line)); // выбор типа прерывания: по фронту
    gpio_ptr->port->INTPOLSET = (0<<(gpio_ptr->line)); // выбор типа прерывания: отрицательному фронту
    gpio_ptr->port->INTEDGESET = (0<<(gpio_ptr->line)); // выбор типа прерывания: по положительному и отрицательному фронту
  }
}

/**
  * @brief  установка значения отдельного GPIO
  * @param  gpio_ptr указатель на программную модель устройства
  * @param  val значения для установки (0 - 0, не 0 - 1)
  */
void gpio_set(type_SINGLE_GPIO* gpio_ptr, uint8_t val)
{
	if (gpio_ptr->port){ //проверка на наличие подобного канала, если 0 - значит канала не существует
		if (val){
			gpio_ptr->port->DATAOUTSET = (1<<(gpio_ptr->line));
		}
		else{
			gpio_ptr->port->DATAOUTCLR = (1<<(gpio_ptr->line));
		}
	}
}

/**
  * @brief  чтение значения GPIO
  * @param  gpio_ptr указатель на программную модель устройства
  * @retval  значение GPIO - 0 или 1
  */
uint8_t gpio_get(type_SINGLE_GPIO* gpio_ptr)
{
  if (gpio_ptr->port){ //проверка на наличие подобного канала, если 0 - значит канала не существует
    return (gpio_ptr->port->DATA >> gpio_ptr->line) & 0x1;
  }
  else {
    return 0;
  }
}

/**
  * @brief  CallBack от обработчика прерывания GPIO
  */
__weak void INT_PORT_CallBack(GPIO_TypeDef *port, uint8_t line)
{
  //
}

/**
  * @brief  Общий обработчик для различных портов
  */
void INT_PORT_Handler(GPIO_TypeDef *port)
{
  for (uint8_t line = 0; line<16; line++){
    if(port->INTSTATUS & (1<<line)){
      port->INTSTATUS = (1<<line);
      //
      INT_PORT_CallBack(port, line);
    }
  }
}

/**
 * @brief Обработка прерываний от IO по фронту для порта L
 * 
 */
void GPIOL_IRQHandler(void){
  INT_PORT_Handler(GPIOL);
}

/**
 * @brief Обработка прерываний от IO по фронту для порта M
 * 
 */
void GPIOM_IRQHandler(void){
  INT_PORT_Handler(GPIOM);
}
