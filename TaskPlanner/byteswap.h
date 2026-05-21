#ifndef _BYTESWAP_H_
#define _BYTESWAP_H_

#include <stdint.h>
#include "main.h"

uint16_t swap_uint16( uint16_t val ); //! Byte swap unsigned short
int16_t swap_int16( int16_t val ) ; //! Byte swap short
uint32_t swap_uint32( uint32_t val ); //! Byte swap unsigned int
int32_t swap_int32( int32_t val ); //! Byte swap int
int64_t swap_int64( int64_t val );
uint64_t swap_uint64( uint64_t val );

#endif
