#ifndef _MKO_RT_H_
#define _MKO_RT_H_

#include "main.h"
#include "CM4/K1921VK028.h"
#include "system_K1921VK028.h"
#include "clock.h"

#define MKO_RT_BUS_A 0
#define MKO_RT_BUS_B 1

#define MKO_RT_MODE_WRITE 0
#define MKO_RT_MODE_READ 1

typedef enum{
	TRES_SUCCESS, TRES_REPLACE, TRES_DMA_ERR, TRES_PROTOCOL_ERR, TRES_BUSY, TRES_FEEDBACK_ERR, 
	TRES_NUM
} type_TRES;

typedef enum{
	TYPE_RX_DATA, TYPE_TX_DATA, TYPE_CMD,
	TYPE_NUM
} type_TYPE;

#pragma pack(push, 2)

// Структура управления без union
#ifndef MILSTD_SIMPLE_STRUCT
#define MILSTD_SIMPLE_STRUCT
typedef struct {
    __IO uint32_t IR;                                                /*!< IR    : type used for word access */
    __IO uint32_t IENR;                                              /*!< IENR    : type used for word access */
    __IO uint32_t Reserved0[2];
    __I uint32_t HCON;                                               /*!< HCON    : type used for word access */
    __IO uint32_t Reserved1[11];
    __IO uint32_t BCSTCON;                                           /*!< BCSTCON    : type used for word access */
    __O uint32_t BCACT;                                              /*!< BCACT    : type used for word access */
    __IO uint32_t BCLNP;                                             /*!< BCLNP    : type used for word access */
    __IO uint32_t BCALNP;                                            /*!< BCALNP    : type used for word access */
    __I uint32_t BCTIM;                                              /*!< BCTIM    : type used for word access */
    __IO uint32_t BCTIMWK;                                           /*!< BCTIMWK    : type used for word access */
    __IO uint32_t BCTRP;                                             /*!< BCTRP    : type used for word access */
    __IO uint32_t BCBSW;                                             /*!< BCBSW    : type used for word access */
    __IO uint32_t Reserved2[2];
    __I uint32_t BCTSP;                                              /*!< BCTSP    : type used for word access */
    __I uint32_t BCATSP;                                             /*!< BCATSP    : type used for word access */
    __IO uint32_t Reserved3[4];
    __I uint32_t RTSTAT;                                             /*!< RTSTAT    : type used for word access */
    __IO uint32_t RTCON;                                             /*!< RTCON    : type used for word access */
    __IO uint32_t RTBST;                                             /*!< RTBST    : type used for word access */
    __IO uint32_t RTSW;                                              /*!< RTSW    : type used for word access */
    __I uint32_t RTSYNC;                                             /*!< RTSYNC    : type used for word access */
    __IO uint32_t RTSADDR;                                           /*!< RTSADDR    : type used for word access */
    __IO uint32_t RTMOD;                                             /*!< RTMOD    : type used for word access */
    __IO uint32_t Reserved4[2];
    __IO uint32_t RTTIM;                                             /*!< RTTIM    : type used for word access */
    __IO uint32_t Reserved5;
    __IO uint32_t RTELMSK;                                           /*!< RTELMSK    : type used for word access */
    __IO uint32_t RTELP;                                             /*!< RTELP    : type used for word access */
    __I uint32_t RTELIP;                                             /*!< RTELIP    : type used for word access */
    __IO uint32_t Reserved6[2];
    __I uint32_t BMSTAT;                                             /*!< BMSTAT    : type used for word access */
    __IO uint32_t BMCON;                                             /*!< BMCON    : type used for word access */
    __IO uint32_t BMADF;                                             /*!< BMADF    : type used for word access */
    __IO uint32_t BMSADF;                                            /*!< BMSADF    : type used for word access */
    __IO uint32_t BMMODF;                                            /*!< BMMODF    : type used for word access */
    __IO uint32_t BMLBS;                                             /*!< BMLBS    : type used for word access */
    __IO uint32_t BMLBE;                                             /*!< BMLBE    : type used for word access */
    __IO uint32_t BMLBP;                                             /*!< BMLBP    : type used for word access */
    __IO uint32_t BMTIM;                                             /*!< BMTIM    : type used for word access */
} MILSTD_TypeDef_Simple;
#endif

typedef struct {
	uint32_t  txsz    : 5;
	uint32_t  txirq   : 1;
	uint32_t  txlog   : 1;
	uint32_t  txen    : 1;
	uint32_t  rxsz    : 5;
	uint32_t  rxirq   : 1;
	uint32_t  rxlog   : 1;
	uint32_t  rxen    : 1;
	uint32_t  bcrxe   : 1;
	uint32_t  igndv   : 1;
	uint32_t  wrap    : 1;
	uint32_t  res1    : 13;
} typeMKORT_CtrlWord;

typedef struct {
	uint32_t  tres    : 3;
	uint32_t  sz      : 6;
	uint32_t  bc      : 1;
	uint32_t  time    : 16;
	uint32_t  res1    : 4;
	uint32_t  irqen   : 1;
	uint32_t  dv      : 1;
} typeMKORT_DescStat;

typedef struct {
	typeMKORT_DescStat DescStat;
	uint32_t  BuffAddr;
	uint32_t  NextPtr;
	uint32_t  res1;
} typeMKORT_Desc;



typedef struct {
	typeMKORT_CtrlWord CtrlWord;
	volatile typeMKORT_Desc  *DescTx;
	volatile typeMKORT_Desc  *DescRx;
	uint32_t res1;
} typeMKORT_SubaddrTbl;



typedef union {
	struct {
		uint32_t  tres    : 3;  // результат передачи  b000 - Ok
		uint32_t  sz      : 6;  // кол-во переданных/принятых слов
		uint32_t  bc      : 1;  // групповое сообщение
		uint32_t  timel   : 14; // таймер
		uint32_t  samc    : 5;  // подадрес/код команды
		uint32_t  type    : 2;  // тип : 0 - передача, 1 - прием, 2 - команда
		uint32_t  irqsr   : 1;
	} uf;
	uint32_t u32;
} typeMKORT_Log;


typedef union
{
	uint16_t whole;
	struct
	{
		uint16_t leng : 5;
		uint16_t sub_addr : 5;
		uint16_t rd_wr : 1;
		uint16_t addr : 5;
	} field;
}typeMKORTCommandWord;

typedef union
{
	uint16_t whole;
	struct
	{
		uint16_t malfunction : 1;
		uint16_t ctrl_ackn : 1;
		uint16_t abonent_malfunction : 1;
		uint16_t busy : 1;
		uint16_t broad_cmd : 1;
		uint16_t not_used : 2;
		uint16_t mem_empty : 1;
		uint16_t service_ack : 1;
		uint16_t must_be_zero : 1;
		uint16_t error : 1;
		uint16_t addr : 5;
	} field;
}typeMKORTAnswerWord;

typedef struct{
	typeMKORT_Log log;
	uint16_t data[32];
}typeMPI_RT_Fifo_Data;

typedef struct{
	typeMPI_RT_Fifo_Data array[8];
	typeMPI_RT_Fifo_Data buffer;
	typeMPI_RT_Fifo_Data last_rec;
	uint8_t wr_ptr, rd_ptr;
	uint8_t rec_num, rec_num_max;
	uint32_t rec_full;
	uint32_t rec_lost;
	uint8_t ena;
} typeMPIRT_RX_Fifo;


typedef struct
{
	MILSTD_TypeDef_Simple *inst;
	//
	volatile typeMKORT_SubaddrTbl *MKORT_SubaddrTbl;
	volatile typeMKORT_Desc  *MKORT_Desc;		
	volatile typeMKORT_Log* MKORT_Log_ptr;
	volatile uint16_t *MKORT_Buff;
	//
	uint8_t addr;
	uint8_t used_bus;
	//
	typeMKORT_Log log_msg;
	uint8_t sa_rx;	//ПА с которым последним работали
	uint16_t sa_rx_data[32]; //последние данные, записанный в ПА
	uint16_t rx_cnt, tx_cnt, cmd_cnt;
	// fifo принятых сообщений для парирования проблем потерь
	typeMPIRT_RX_Fifo rx_fifo;
	//
	uint16_t irq_cnter;
	//
	uint8_t error;
	uint8_t error_cnt;
}typeMKORTStruct;

#pragma pack(pop)

int8_t mko_rt_init(typeMKORTStruct *mko_rt_ptr, uint8_t mko_addr);
void mko_rt_set_busy(typeMKORTStruct *mko_rt_ptr);
void mko_rt_release_busy(typeMKORTStruct *mko_rt_ptr);
void mko_rt_set_aw_bit_7(typeMKORTStruct *mko_rt_ptr, uint8_t val);
void mko_rt_clear_data(typeMKORTStruct *mko_rt_ptr);
uint8_t mko_rt_transaction_handler(typeMKORTStruct *mko_rt_ptr, uint8_t* sa_ptr);
void mko_rt_write_to_subaddr(typeMKORTStruct *mko_rt_ptr, uint8_t subaddr, uint16_t* data);
void mko_rt_read_from_subaddr(typeMKORTStruct *mko_rt_ptr, uint8_t subaddr, uint16_t* data);
//
int8_t mko_rt_rx_fifo_init(typeMKORTStruct *mko_rt_ptr);
int8_t mko_rt_write_rx_fifo(typeMKORTStruct *mko_rt_ptr, typeMPI_RT_Fifo_Data* data);
int8_t mko_rt_read_rx_fifo(typeMKORTStruct *mko_rt_ptr, typeMPI_RT_Fifo_Data* data);
void mko_rt_rx_fifo_rec_num_update(typeMKORTStruct* mko_rt_ptr);
//
uint8_t mko_rt_get_addr_from_gpio(typeMKORTStruct *mko_rt_ptr);
void __mko_rt_cmd_msg(typeMKORTStruct *mko_rt_ptr, uint8_t cmd_num);
//
void mko_rt_get_error(typeMKORTStruct *mko_rt_ptr, uint8_t* error, uint8_t* error_cnt);

void mko_rt_irq_rx_callback(typeMKORTStruct *mko_rt_ptr);

#endif
