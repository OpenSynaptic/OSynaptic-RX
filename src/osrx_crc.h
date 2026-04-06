#ifndef OSRX_CRC_H
#define OSRX_CRC_H

/*
 * osrx_crc.h -- CRC-8/SMBUS and CRC-16/CCITT-FALSE for OSynaptic-RX (C89).
 *
 * Both algorithms are bit-loop implementations (no lookup table) to keep
 * Flash usage below 100 bytes each on AVR.
 *
 * The same polynomials and init values are used on the TX side so that
 * received frames can be validated directly.
 */

#include "osrx_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * CRC-8/SMBUS over a data buffer.
 *   poly  = 0x07u  (x^8 + x^2 + x + 1)
 *   init  = 0x00u  (pass as init parameter)
 *   Standard check value for "123456789" = 0xF4
 *
 * Returns 0 when data is NULL or len <= 0.
 */
osrx_u8  osrx_crc8 (const osrx_u8 *data, int len,
                    osrx_u16 poly, osrx_u8 init);

/*
 * CRC-16/CCITT-FALSE over a data buffer.
 *   poly  = 0x1021u  (x^16 + x^12 + x^5 + 1)
 *   init  = 0xFFFFu  (pass as init parameter)
 *   Standard check value for "123456789" = 0x29B1
 *
 * Returns 0 when data is NULL or len <= 0.
 */
osrx_u16 osrx_crc16(const osrx_u8 *data, int len,
                    osrx_u16 poly, osrx_u16 init);

#ifdef __cplusplus
}
#endif

#endif /* OSRX_CRC_H */
