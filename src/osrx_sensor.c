/* osrx_sensor.c -- Sensor field parser for OSynaptic-RX (C89) */
#include "osrx_sensor.h"
#include "osrx_b62.h"

#include <string.h>

int osrx_sensor_unpack(
    const osrx_u8    *body,
    int               body_len,
    osrx_sensor_field *out
) {
    int p;
    int seg_len;
    int ok;
    int i;
    int sid_start;
    int gt_pos;   /* position of '>' separator                            */
    int dot_pos;  /* position of '.' after state                          */
    int col_pos;  /* position of ':' after unit                           */
    int end_pos;  /* position of trailing '|'                             */

    if (!body || body_len <= 0 || !out) { return 0; }

    /*
     * Wire body format (OpenSynaptic):
     *   "{aid}.{status}.{ts_b64}|{sid}>{state}.{unit}:{b62}|"
     *
     * Step 1: Skip the body header — find the first '|' that terminates
     *         the "{aid}.{status}.{ts_b64}" segment.
     */
    p = 0;
    while (p < body_len && body[p] != (osrx_u8)'|') { ++p; }
    if (p >= body_len) { return 0; } /* no header sentinel */
    ++p;  /* p now points at the start of the sensor segment */

    sid_start = p;

    /* Step 2: Find '>' — marks end of sensor_id. */
    gt_pos = p;
    while (gt_pos < body_len && body[gt_pos] != (osrx_u8)'>') { ++gt_pos; }
    if (gt_pos >= body_len)          { return 0; }
    if (gt_pos <= sid_start)         { return 0; } /* empty sid */

    /* --- sensor_id --------------------------------------------------- */
    seg_len = gt_pos - sid_start;
    if (seg_len <= 0 || seg_len >= OSRX_ID_MAX) { return 0; }
    for (i = 0; i < seg_len; ++i) {
        out->sensor_id[i] = (char)body[sid_start + i];
    }
    out->sensor_id[seg_len] = '\0';

    /* Step 3: Skip the state token — find '.' after '>'. */
    dot_pos = gt_pos + 1;
    while (dot_pos < body_len && body[dot_pos] != (osrx_u8)'.') { ++dot_pos; }
    if (dot_pos >= body_len) { return 0; } /* no state/unit separator */

    /* Step 4: Find ':' — marks end of unit code. */
    col_pos = dot_pos + 1;
    while (col_pos < body_len && body[col_pos] != (osrx_u8)':') { ++col_pos; }
    if (col_pos >= body_len) { return 0; }

    /* --- unit -------------------------------------------------------- */
    seg_len = col_pos - (dot_pos + 1);
    if (seg_len <= 0 || seg_len >= OSRX_UNIT_MAX) { return 0; }
    for (i = 0; i < seg_len; ++i) {
        out->unit[i] = (char)body[dot_pos + 1 + i];
    }
    out->unit[seg_len] = '\0';

    /* Step 5: Find trailing '|' — marks end of b62 value. */
    end_pos = col_pos + 1;
    while (end_pos < body_len && body[end_pos] != (osrx_u8)'|') { ++end_pos; }
    /* Trailing '|' may be absent if body_len was trimmed; still valid. */

    /* --- b62 value --------------------------------------------------- */
    seg_len = end_pos - (col_pos + 1);
    if (seg_len <= 0 || seg_len >= OSRX_B62_MAX) { return 0; }

    out->scaled = osrx_b62_decode(
        (const char *)(body + col_pos + 1), seg_len, &ok
    );
    if (!ok) { return 0; }

    return 1;
}

int osrx_sensor_recv(
    const osrx_u8    *packet,
    int               packet_len,
    osrx_packet_meta *out_meta,
    osrx_sensor_field *out_field
) {
    if (!osrx_packet_decode(packet, packet_len, out_meta)) { return 0; }
    if (!out_meta->crc8_ok || !out_meta->crc16_ok)        { return 0; }
    return osrx_sensor_unpack(
        packet + out_meta->body_off,
        out_meta->body_len,
        out_field
    );
}
