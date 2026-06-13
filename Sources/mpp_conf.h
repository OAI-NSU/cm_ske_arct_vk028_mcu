#ifndef _MPP_CONF_H_
#define _MPP_CONF_H_

// Настройки каналов МПП
#define MPP_DEV_NUM (8)
// !!настройки уставкии МПП (offset) количество должно совпадать с MPP_DEV_NUM!!
#define MPP_DEF_OFFSET {0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30}  //TODO: проверить правильные значения уставок
// адреса МПП на внутренней шине
#define MPP_ID {4, 4, 5, 5, 6, 6, 7, 7}
// номер канала, используемый устройством МПП
#define MPP_CHAN_NUM {0, 1, 0, 1, 0, 1, 0, 1}

#define MPP_CHAN_K {1.0E0, 1.0E0, 1.0E0, 1.0E0, 1.0E0, 1.0E0, 1.0E0, 1.0E0}
#define MPP_CHAN_B {0.0E0, 0.0E0, 0.0E0, 0.0E0, 0.0E0, 0.0E0, 0.0E0, 0.0E0}

#endif
