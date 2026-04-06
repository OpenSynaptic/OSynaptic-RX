/* osrx_crc.c -- CRC-8 and CRC-16 for OSynaptic-RX (C89) */
#include "osrx_crc.h"

osrx_u8 osrx_crc8(const osrx_u8 *data, int len, osrx_u16 poly, osrx_u8 init)
{
    osrx_u8 crc;
    int     i;
    int     bit;

    if (!data || len <= 0) { return 0; }

    crc = init;
    for (i = 0; i < len; ++i) {
        crc = (osrx_u8)(crc ^ data[i]);
        for (bit = 0; bit < 8; ++bit) {
            if (crc & 0x80u) {
                crc = (osrx_u8)((crc << 1) ^ (osrx_u8)poly);
            } else {
                crc = (osrx_u8)(crc << 1);
            }
        }
    }
    return crc;
}

osrx_u16 osrx_crc16(const osrx_u8 *data, int len, osrx_u16 poly, osrx_u16 init)
{
    osrx_u16 crc;
    int      i;
    int      bit;

    if (!data || len <= 0) { return 0; }

    crc = init;
    for (i = 0; i < len; ++i) {
        crc = (osrx_u16)(crc ^ ((osrx_u16)data[i] << 8));
        for (bit = 0; bit < 8; ++bit) {
            if (crc & 0x8000u) {
                crc = (osrx_u16)((crc << 1) ^ poly);
            } else {
                crc = (osrx_u16)(crc << 1);
            }
        }
    }
    return crc;
}
