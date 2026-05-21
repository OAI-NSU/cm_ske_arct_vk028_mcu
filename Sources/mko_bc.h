#ifndef _MKO_BC_H_
#define _MKO_BC_H_

#include "main.h"
#include "CM4/K1921VK028.h"
#include "system_K1921VK028.h"
#include "timers.h"

#define MKO_BC_BUS_A 0
#define MKO_BC_BUS_B 1

#define MKO_BC_MODE_WRITE 0
#define MKO_BC_MODE_READ 1

typedef enum{
	TFRST_SUCCESS, TFRST_RT_NANSW, TFRST_RT2_NANSW, TFRST_ERROR_BITS, 
	TFRST_PROTOCOL_ERR, TFRST_DESC_ERR, TFRST_DMA_ERR, TFRST_LB_ERR, 
	TFRST_NUM
} type_TFRST;


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

// Переменные от АА для задания дискрипторов
typedef union {
	struct {
		uint32_t  stime   : 16;
		uint32_t  res1    : 2;
		uint32_t  gap     : 1;
		uint32_t  stbus   : 1;
		uint32_t  nret    : 3;
		uint32_t  retmd   : 2;
		uint32_t  susn    : 1;
		uint32_t  suse    : 1;
		uint32_t  irqn    : 1;
		uint32_t  irqe    : 1;
		uint32_t  excl    : 1;
		uint32_t  wtrig   : 1;
		uint32_t  res2    : 1;
	} uf;
	uint32_t  u32;
} typeMKOBC_DescWord0;

typedef union {
	struct {
		uint32_t  wcmc    : 5;
		uint32_t  rtsa1   : 5;
		uint32_t  tr      : 1;
		uint32_t  rtad1   : 5;
		uint32_t  rtsa2   : 5;
		uint32_t  rtad2   : 5;
		uint32_t  rtto    : 4;
		uint32_t  bus     : 1;
		uint32_t  dum     : 1;
	} uf;
	uint32_t  u32;
} typeMKOBC_DescWord1;

typedef union {
	struct {
		uint32_t  tfrst   : 3;
		uint32_t  res1    : 1;
		uint32_t  retcnt  : 4;
		uint32_t  rtst    : 8;
		uint32_t  rt2st   : 8;
		uint32_t  res2    : 7;
		uint32_t  flg0    : 1;
	} uf;
	uint32_t  u32;
} typeMKOBC_DescResult;

typedef union {
	struct {
		uint32_t  stcc    : 8;
		uint32_t  rtcc    : 8;
		uint32_t  rt2cc   : 8;
		uint32_t  mode    : 1;
		uint32_t  act     : 1;
		uint32_t  irqc    : 1;
		uint32_t  res1    : 4;
		uint32_t  flg_one : 1;
	} uf;
	uint32_t  u32;
} typeMKOBC_DescBranchCond;

typedef struct
{
	typeMKOBC_DescWord0 Word0;
	typeMKOBC_DescWord1 Word1;
	uint32_t BuffAddr;
	typeMKOBC_DescResult Result;
	typeMKOBC_DescBranchCond BranchCond;
	uint32_t BranchAddr;
	uint32_t res1;
	uint32_t res2;
} typeMKOBC_DescList;

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
}typeCommandWord;

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
}typeAnswerWord;

typedef struct
{
	MILSTD_TypeDef_Simple *inst;
	//
	volatile typeMKOBC_DescList* MKOBC_DescListRx_ptr;
	volatile typeMKOBC_DescList* MKOBC_DescListTx_ptr;
	//
	uint16_t MKOBC_BuffRx[32];
	uint16_t MKOBC_BuffTx[32];
	//
	uint8_t used_bus;
	//
	typeCommandWord cw;
	typeAnswerWord aw;
	//
	uint8_t error;
	uint8_t error_cnt;
	//
	int need_to_process_flag;
}typeMKOBCStruct;

#pragma pack(pop)

void mko_bc_init(typeMKOBCStruct *mko_ptr);
void mko_bc_set_bus(typeMKOBCStruct *mko_ptr, uint8_t bus);
uint32_t mko_bc_rx(typeMKOBCStruct *mko_ptr, uint8_t rt_addr, uint8_t subaddr, uint8_t leng, uint16_t *buff);
uint32_t mko_bc_tx(typeMKOBCStruct *mko_ptr,uint8_t rt_addr, uint8_t subaddr, uint8_t leng, uint16_t *buff);
uint8_t mko_bc_transaction_start_with_bus_find(typeMKOBCStruct *mko_ptr, uint8_t mode, uint8_t addr, uint8_t subaddr, uint16_t* data, uint8_t leng);
uint8_t mko_bc_transaction_start(typeMKOBCStruct *mko_ptr, uint8_t mode, uint8_t addr, uint8_t subaddr, uint16_t* data, uint8_t leng);
uint8_t mko_bc_transaction_start_by_cw(typeMKOBCStruct *mko_ptr, uint16_t cw_var, uint16_t* data);
uint8_t mko_bc_change_bus(typeMKOBCStruct *mko_ptr);
//
void mko_bc_get_error(typeMKOBCStruct *mko_ptr, uint8_t* error, uint8_t* error_cnt);

#endif
