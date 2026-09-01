#include "main.h"
#include "gpio.h"
#include "timers.h"
#include "clock.h"
#include "adc.h"
#include "mko_bc.h"
#include "mko_rt.h"
#include "wdt.h"
#include "frame_mem.h"
//
#include "retarget_conf.h"  // перенаправление printf в UART3
#include "task_planner.h"
#include "power_management.h"
#include "internal_bus.h"
#include "cm.h"
#include "dep.h"
#include "dir.h"
#include "mpp_conf.h"
#include "mpp.h"
#include "gen_mpp.h"
#include "pcu.h"
// #include "dir.h"

//***Общие настройки***//

//версия прошивки
#define CM_SW_VERSION 			  "0.06"
// номер устройства
#define FRAME_DEV_ID 			    235 // 235 236 237 238 239

//
extern uint32_t SystemCoreClock;  /*100_000_000*/
// создание объектов отдельных программных моделей 
type_GPIO_OAI_cm gpio;
typeADCStruct adc;
typeTPStruct tp;
typeCMModel cm; 
type_GPIO_PCU pcu;
typePower pwr;
typeIBStruct ib;
type_STM_Model stm;
typeMKOBCStruct mko_bc;
//
typeMPPStruct mpp[MPP_DEV_NUM];   //! программная модель периферийных модулей МПП
typeGEN_MPP_Struct gmpp;          
typeDEPStruct dep[8];
// typeMKKDStruct mkkd;
//
void SystemCoreClockUpdate(void);

// прототипы функций для использования внутри main.c (описание в функциях)
void __main_process_registration_box(void);
void __main_init_peripheral_modules(void);
void __main_base_init(void);

/**
 * @brief суперцикл
 * правка только с согласованием с автором проекта
 * остальные правки на свой страх и риск
 * @return int никогда не возвращает, так как работает бесконечно
 */
int main() {
  // базовая инициализация
  SystemCoreClockUpdate();
  retarget_init();
  SysTick_Config(SystemCoreClock/1000);  // 1 ms  interrupt period
  //
  __main_base_init();
  //
	WDT_Init();
  //
  while(1) {
    tp_handler(&tp); // обработка планировщика задач
    WDRST;
  }/*while(1)*/  
}

/**
  * @brief  обертка для инициализации всей периферии
*/
void __main_init_peripheral_modules(void)
{
	// настройки каналов МПП
	uint32_t mpp_offsets_array[MPP_DEV_NUM] = MPP_DEF_OFFSET;
	uint32_t mpp_ib_id[MPP_DEV_NUM] = MPP_ID;
	uint32_t mpp_ch_num[MPP_DEV_NUM] = MPP_CHAN_NUM;
	float mpp_ch_k[MPP_DEV_NUM] = MPP_CHAN_K;
	float mpp_ch_b[MPP_DEV_NUM] = MPP_CHAN_B;

  uint32_t dep_id[DEP_AD8 - DEP_AD1 + 1] = { 1, 2, 3, 4, 5, 6, 7, 8};
  uint32_t dep_sa[DEP_AD8 - DEP_AD1 + 1] = {  CM_MKO_SA_DEP_AD1, 
                                              CM_MKO_SA_DEP_AD2, 
                                              CM_MKO_SA_DEP_AD3, 
                                              CM_MKO_SA_DEP_AD4, 
                                              CM_MKO_SA_DEP_AD5, 
                                              CM_MKO_SA_DEP_AD6, 
                                              CM_MKO_SA_DEP_AD7, 
                                              CM_MKO_SA_DEP_AD8};

	uint8_t uint8_var = 0;
	/**
	 * @brief  цикл используется для последовательной нумерации схожих устройств. Использование необязательно
	 */
	for (uint8_var = 0; uint8_var<MPP_DEV_NUM; uint8_var++){
    if(MPP1 + uint8_var <= MPP_DEV_NUM){
      mpp_init(	&mpp[uint8_var], 
                uint8_var, 
                mpp_ib_id[uint8_var], 
                FRAME_DEV_ID, 
                CM_MKO_SA_MPP1 + uint8_var, 
                mpp_ch_num[uint8_var], 
                mpp_offsets_array[uint8_var], 
                cm.ib_ptr, 
                &cm.global_frame_num,
                MPP_TYPE_MPP);
      mpp_set_calibr(&mpp[uint8_var], mpp_ch_k[uint8_var], mpp_ch_b[uint8_var]);
    }
	}
  gmpp_init(&gmpp, GMPP, FRAME_DEV_ID, CM_MKO_SA_MATRIX, CM_MKO_SA_WIND, &cm.global_frame_num, mpp);
	
  for (uint8_var = DEP_AD1; uint8_var <= DEP_AD8; uint8_var++){
    dep_init( &dep[uint8_var - DEP_AD1], 
              uint8_var, 
              dep_id[uint8_var - DEP_AD1], 
              FRAME_DEV_ID, 
              dep_sa[uint8_var - DEP_AD1],
              cm.ib_ptr,
              cm.mko_bc_ptr,
              &cm.global_frame_num,
              DEP_MODE_MKO );
  }
	// mkkd_init(&mkkd, MKKD, 10, FRAME_DEV_ID, CM_MKO_SA_DND, cm.ib_ptr, &cm.global_frame_num);
}

// Командный интерфейс
/**
  * @brief  обертка для регистрации всех необходимых процессов
*/
void __main_process_registration_box(void)
{
	// процессы для поддержания работы ЦМ и его систем: питания, АЦП, внутренней шины
	tp_process_registration(&tp, &cm_process_tp, &cm, CM*128, TP_SHARED_MEM_VOL_B);
	tp_process_registration(&tp, &ib_process_tp, &ib, 0, 0);
	tp_process_registration(&tp, &adc_process_tp, &adc, 0, 0);
	tp_process_registration(&tp, &pwr_process_tp, &pwr, 0, 0);
  tp_process_registration(&tp, &stm_process_tp, &stm, 0, 0);
  tp_process_registration(&tp, &pcu_process_tp, &pcu, 0, 0);

	// Процессы периферийных устройств
	for (uint8_t uint8_var = 0; uint8_var < MPP_DEV_NUM; uint8_var++){
		tp_process_registration(&tp, &mpp_process_tp, &mpp[uint8_var], 128*(MPP1+uint8_var), 128);
	}

  for (uint8_t uint8_var = 0; uint8_var < 8; uint8_var++){
		tp_process_registration(&tp, &dep_process_tp, &dep[uint8_var], 128*(DEP_AD1+uint8_var), 128);
	}
  // tp_process_registration(&tp, &gmpp_process_tp, &gmpp, 128*GMPP, 128);
  //TODO: добавить МККД
	// tp_process_registration(&tp, &dir_process_tp, &dir, 128*(DIR), 128);
}

/**
  * @brief  обертка для базовой инициализации БЭ через команду МКО (или ВШ)
*/
void __main_base_init(void)
{
	printf("\nCM Init: start\n");
	// отключение периферии
  
  //
  printf("SW v%s\n", CM_SW_VERSION);
	printf("\n______ \\_(o_O)_/ ______\n\n");
	printf("CM_SKE_ARKTIKA (1921VK028 based) frame_dev_id <%d>\n", FRAME_DEV_ID);
	//
	oai_cm_gpio_init(&gpio);
	clock_init(0);
	printf("%s: clock init \n", now());
	Timers_Init();
  clock_set_time_s(0);
  Timer_Delay(0, 100);
	printf("%s: clock time after 100 ms delay (OK - is about 0.1 s)\n", now());
	// инициализация структур
  printf("%s: gpio init\n", now());
  mko_bc_init(&mko_bc);
  printf("%s: mko_bc init\n", now());
  //
  stm_init(&stm, &gpio);
  printf("%s: stm init\n", now());
  //
  ib_init(&ib);
  printf("%s: internal bus init\n", now());
  adc_init(&adc);
  printf("%s: adc init\n", now());
  pwr_init(&pwr, &adc, &gpio); 
  printf("%s: pwr init\n", now());
  pcu_init(&pcu);
  printf("%s: pcu init\n", now());
  //
  //TODO: добавить инициализацию МПИ
  //
	cm_init(  &cm, 
            CM,
            CM_SELF_MB_ID,
            &mko_bc,
						&pcu,
            &ib,
            &pwr,
            &gpio.io[14],
            &stm,
            FRAME_DEV_ID,
            CM_SW_VERSION,
            CM+1);  // инициализация объекта программной модели ЦМ (включает в себя все периферийные модели ЦМ: ВШ, МКО, GPIO, ADC и т.п.)
	// включение периферии
	__main_init_peripheral_modules();
	// отключение планировщика задач
	tp_init(&tp);
	printf("%s: TP init done \n", now());
	//
	WDRST;
	// сброс времени
	clock_set_time_s(0);
	// регистрация процессов
	__main_process_registration_box();
}

void cm_pcu_command_interface_handler(typeCMModel *cm_ptr)
{
  uint16_t cmd_num = 0;
  uint32_t default_intervals[CM_INTERV_NUMBER] = DEFAULT_CM_INTERV_VALUES_S;
  uint32_t intervals_60[CM_INTERV_NUMBER] = {10, 	60, 	2, 		10, 	1};
  uint32_t dbg_intervals[CM_INTERV_NUMBER] = {5, 	10, 	2, 		10, 	1};
  //
  cmd_num = pcu_cmd_extraction(cm_ptr->pcu_ptr);
  if (cmd_num){
    switch (cmd_num){
      case 0x0101:
        printf("%s: pcu cmd <0x%04X> \n", now(), cmd_num);
        break;
			case 0x0201:
				cm_set_speedy_mode(cm_ptr, 0xFFFF, 0);
        for (uint8_t i_num = 0; i_num < CM_INTERV_NUMBER; i_num++) {cm_set_interval_value(cm_ptr, i_num, default_intervals[i_num]);}
        printf("%s: pcu cmd <0x%04X> - default intervals \n", now(), cmd_num);
        break;
			case 0x0202:
				cm_set_speedy_mode(cm_ptr, 0xFFFF, 0);
        for (uint8_t i_num = 0; i_num < CM_INTERV_NUMBER; i_num++) {cm_set_interval_value(cm_ptr, i_num, intervals_60[i_num]);}
        printf("%s: pcu cmd <0x%04X> - meas interval to 60s \n", now(), cmd_num);
        break;
			case 0x0203:
				cm_set_speedy_mode(cm_ptr, 0xFFFF, 0);
        for (uint8_t i_num = 0; i_num < CM_INTERV_NUMBER; i_num++) {cm_set_interval_value(cm_ptr, i_num, dbg_intervals[i_num]);}
        printf("%s: pcu cmd <0x%04X> - dbg intervals \n", now(), cmd_num);
        break;
			case 0x0204:
        cm_set_speedy_mode(cm_ptr, 0xFFFF, 3600);
        printf("%s: pcu cmd <0x%04X> - speedy mode \n", now(), cmd_num);
        break;
      default:
        printf("%s: unknown pcu cmd \n", now());
        break;
    }
  }
}

/**
 * @brief обработчик отладочных команд через ВШ
 * 
 * @param cm_ptr указатель на объект управления ЦМ
 */
void cm_dbg_ib_command_handler(typeCMModel* cm_ptr)
{
	// typeFrameStruct frame;
	//
	if (cm_ptr->ib_ptr->command_frame_flag){
		cm_ptr->ib_ptr->command_frame_flag = 0;
		if (cm_ptr->ib_ptr->command_frame.dev_id == CM_SELF_MB_ID){
			if (cm_ptr->ib_ptr->command_frame.f_code == MB_F_CODE_16){
				printf("CM DBG CMD: ID <%d>, FC <%d>, RA <%d> \n", cm_ptr->ib_ptr->command_frame.dev_id, cm_ptr->ib_ptr->command_frame.f_code, cm_ptr->ib_ptr->command_frame.reg_addr);
				switch(cm_ptr->ib_ptr->command_frame.reg_addr){
					case CM_DBG_CMD_SWITCH_ON_OFF:
						cm_ptr->ib_ptr->global_dbg_flag = __REV16(cm_ptr->ib_ptr->command_frame.data[0]) & 0x01;
						break;
					case CM_DBG_CMD_CM_RESET:
						printf("Rst by WDG ");
						Timer_Delay(1, 100000);
						printf("Error\n");
						break;
					case CM_DBG_CMD_CM_CHECK_MEM:
						fr_mem_check(&cm_ptr->mem);
						break;
					case CM_DBG_CMD_CM_INIT:
						if (__REV16(cm_ptr->ib_ptr->command_frame.data[0]) == 0xAA55) __main_base_init();
						break;
					case CM_DBG_CMD_ARCH_REQUEST:
            //
						break;
				}
			}
		}
	}
}

void ADC_SEQ0_CallBack(void)
{
  volatile int i;
  ADC->IC = 1;  //interrupt flag reset
  for(i=0; i<ADC_CHANNEL_NUM; i++)  adc.data[i] = ADC->SEQ[0].SFIFO; // (первое чтение - пустое)
}

void SysTick_Handler(void) 
{
  tp_timer_handler(&tp, 1000);
}

void INT_PORT_CallBack(GPIO_TypeDef *port, uint8_t line)
{
  uint8_t irq_source_num = 0;
  irq_source_num = oai_cm_io_find(&gpio, port, line);
  // Важно, в данном блок происходит связь команды управления и выходов (пример: OUT_CMD_1 <-> KU_CMD_WATCH_MODE_ON)
  switch(irq_source_num){
    case OUT_CMD_1:
      pcu_set_event(&pcu, PCU_EVENT_IRQ);
      break;
    case OUT_CMD_2:
      pcu_set_event(&pcu, PCU_EVENT_IRQ);
      break;
    default:
      // обработка прерывания от GPIO
    break;
  }
}

void HardFault_Handler(void)
{
  __DSB();
  while(1){
  }
}

void MemManage_Handler(void)
{
  __DSB();
  while(1){
  }
}
void BusFault_Handler(void)
{
  __DSB();
  while(1){
  }
}
