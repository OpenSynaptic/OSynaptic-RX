#ifndef OSRX_TYPES_H
#define OSRX_TYPES_H

/*
 * OSynaptic-RX  --  C89-compatible portable integer types.
 *
 * Assumptions (valid for avr-gcc, arm-none-eabi-gcc, sdcc, xc8):
 *   unsigned char   == 8 bits
 *   unsigned short  == 16 bits
 *   unsigned long   == 32 bits
 *
 * Host compilers such as LP64 Linux use 64-bit long, so select any
 * available 32-bit base type instead of hard-coding long.
 *
 * If your toolchain differs, define OSRX_U8_DEFINED and provide typedefs
 * before including this header.
 */

#include <limits.h>

#ifndef OSRX_U8_DEFINED
typedef unsigned char   osrx_u8;
typedef unsigned short  osrx_u16;
typedef signed   char   osrx_i8;
typedef signed   short  osrx_i16;

#if ULONG_MAX == 0xffffffffUL
typedef unsigned long   osrx_u32;
typedef signed   long   osrx_i32;
#elif UINT_MAX == 0xffffffffU
typedef unsigned int    osrx_u32;
typedef signed   int    osrx_i32;
#else
#error "OSynaptic-RX requires a 32-bit integer type for osrx_u32/osrx_i32"
#endif

#define OSRX_U8_DEFINED 1
#endif

#ifndef NULL
#  include <stddef.h>
#endif

#endif /* OSRX_TYPES_H */
