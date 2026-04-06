#ifndef OSRX_TYPES_H
#define OSRX_TYPES_H

/*
 * OSynaptic-RX  --  C89-compatible portable integer types.
 *
 * <stdint.h> is C99; we derive equivalent types from first principles.
 *
 * Assumptions (valid for avr-gcc, arm-none-eabi-gcc, sdcc, xc8):
 *   unsigned char   == 8 bits
 *   unsigned short  == 16 bits
 *   unsigned long   == 32 bits
 *
 * If your toolchain differs, define OSRX_U8_DEFINED and provide typedefs
 * before including this header.
 */

#ifndef OSRX_U8_DEFINED
typedef unsigned char   osrx_u8;
typedef unsigned short  osrx_u16;
typedef unsigned long   osrx_u32;
typedef signed   char   osrx_i8;
typedef signed   short  osrx_i16;
typedef signed   long   osrx_i32;
#define OSRX_U8_DEFINED 1
#endif

#ifndef NULL
#  include <stddef.h>
#endif

#endif /* OSRX_TYPES_H */
