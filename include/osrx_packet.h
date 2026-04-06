#ifndef OSRX_PACKET_H
#define OSRX_PACKET_H

#include "osrx_types.h"
#include "osrx_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Parsed header fields extracted from an incoming OpenSynaptic wire packet.
 */
typedef struct osrx_packet_meta {
    osrx_u8  cmd;         /* command byte                                  */
    osrx_u8  route_count; /* should be 1 for sensor nodes                  */
    osrx_u32 aid;         /* source device ID (big-endian u32 in wire)     */
    osrx_u8  tid;         /* template / transaction ID                     */
#if !OSRX_NO_TIMESTAMP
    osrx_u32 ts_sec;      /* lower 32 bits of the 48-bit wire timestamp    */
#endif
    int      body_off;    /* byte offset of body inside the raw packet     */
    int      body_len;    /* body length in bytes                          */
    int      crc8_ok;     /* 1 = CRC-8  of body  verified, 0 = mismatch   */
    int      crc16_ok;    /* 1 = CRC-16 of frame verified, 0 = mismatch   */
} osrx_packet_meta;

/*
 * Decode a raw byte buffer into osrx_packet_meta.
 *
 * The function validates both CRC fields and fills 'out_meta'.
 * On success, out_meta->body_off and out_meta->body_len indicate
 * where the body is inside 'packet' so the caller can pass it to
 * osrx_sensor_unpack() without copying.
 *
 * Returns 1 on success (packet structurally valid; check crc*_ok for
 * integrity), 0 if packet is NULL, too short, or header is malformed.
 */
int osrx_packet_decode(
    const osrx_u8  *packet,
    int             packet_len,
    osrx_packet_meta *out_meta
);

#ifdef __cplusplus
}
#endif

#endif /* OSRX_PACKET_H */
