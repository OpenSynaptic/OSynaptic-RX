#ifndef OSRX_CRC_H
#define OSRX_CRC_H

#include "osrx_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* CRC-8  (poly=0x07, init=0x00  -- CRC-8/SMBUS) */
osrx_u8  osrx_crc8 (const osrx_u8 *data, int len, osrx_u16 poly, osrx_u8  init);

/* CRC-16 (poly=0x1021, init=0xFFFF -- CRC-16/CCITT-FALSE) */
osrx_u16 osrx_crc16(const osrx_u8 *data, int len, osrx_u16 poly, osrx_u16 init);

#ifdef __cplusplus
}
#endif

#endif /* OSRX_CRC_H */
