#ifndef _STM_SETTINGS_H_
#define _STM_SETTINGS_H_

#include "main.h"

/**
  * @brief  нумерация STM-сигналов, используемых в данном устройстве
*/
typedef enum stm_list
{
	KPBE, NORMCM, AMKO, 
	STM_NUM
} stm_list;

#define STM_CM_IO_NUM 		    {0, 1, 2}
#define STM_DEFAULT_VAL 	  	{1, 0, 0}

#endif

