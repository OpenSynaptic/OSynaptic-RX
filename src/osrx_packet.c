/* osrx_packet.c -- Wire packet header decoder for OSynaptic-RX (C89) */
#include "osrx_packet.h"
#include "osrx_config.h"
#if OSRX_VALIDATE_CRC8 || OSRX_VALIDATE_CRC16
#include "osrx_crc.h"
#endif

int osrx_packet_decode(
    const osrx_u8    *packet,
    int               packet_len,
    osrx_packet_meta *out_meta
) {
#if OSRX_VALIDATE_CRC8
    osrx_u8  exp_crc8;
    osrx_u8  got_crc8;
#endif
#if OSRX_VALIDATE_CRC16
    osrx_u16 exp_crc16;
    osrx_u16 got_crc16;
#endif
    int      body_len;

    if (!packet || !out_meta) { return 0; }

    /* Minimum: 13 header + 0 body + 3 CRC = 16 bytes */
    if (packet_len < 16) { return 0; }

    body_len = packet_len - 13 - 3;
    if (body_len < 0 || body_len > OSRX_BODY_MAX) { return 0; }

    /* --- Parse header fields ----------------------------------------- */
    out_meta->cmd         = packet[0];
    out_meta->route_count = packet[1];
    out_meta->aid  = ((osrx_u32)packet[2] << 24)
                   | ((osrx_u32)packet[3] << 16)
                   | ((osrx_u32)packet[4] <<  8)
                   |  (osrx_u32)packet[5];
    out_meta->tid         = packet[6];
#if !OSRX_NO_TIMESTAMP
    /* Timestamp: 6 bytes big-endian; we keep the lower 32 bits (bytes 9-12). */
    out_meta->ts_sec = ((osrx_u32)packet[ 9] << 24)
                     | ((osrx_u32)packet[10] << 16)
                     | ((osrx_u32)packet[11] <<  8)
                     |  (osrx_u32)packet[12];
#endif
    out_meta->body_off = 13;
    out_meta->body_len = body_len;

    /* --- CRC-8: over body only --------------------------------------- */
#if OSRX_VALIDATE_CRC8
    got_crc8 = packet[packet_len - 3];
    if (body_len > 0) {
        exp_crc8 = osrx_crc8(packet + 13, body_len, 0x07u, 0x00u);
    } else {
        exp_crc8 = 0u;   /* CRC-8 of empty = init=0, no bytes processed */
    }
    out_meta->crc8_ok = (exp_crc8 == got_crc8) ? 1 : 0;
#else
    out_meta->crc8_ok = 1;   /* validation skipped */
#endif

    /* --- CRC-16: over all bytes except the last 2 -------------------- */
#if OSRX_VALIDATE_CRC16
    got_crc16 = ((osrx_u16)packet[packet_len - 2] << 8)
              |  (osrx_u16)packet[packet_len - 1];
    exp_crc16 = osrx_crc16(packet, packet_len - 2, 0x1021u, 0xFFFFu);
    out_meta->crc16_ok = (exp_crc16 == got_crc16) ? 1 : 0;
#else
    out_meta->crc16_ok = 1;  /* validation skipped */
#endif

    return 1;
}
