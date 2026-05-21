#ifndef __CRC16_H
#define __CRC16_H

# include <string.h>
# include <stdio.h>
# include <stdint.h>

//#ifndef uint32_t
//typedef unsigned int uint32_t;
//typedef int int32_t;
//typedef unsigned short uint16_t;
//typedef short int16_t;
//typedef unsigned char uint8_t;
//typedef signed char int8_t;
//#endif

uint16_t crc16_oai(uint8_t *buf, uint8_t len);
uint8_t crc8_rmap_data(uint8_t* data, uint8_t len);
uint8_t crc8_rmap_header(uint8_t* data, uint8_t len);
uint16_t norby_crc16_calc(uint8_t *buffer, uint16_t len);

#endif
