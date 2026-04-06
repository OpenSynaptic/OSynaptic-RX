/* osrx_parser.c -- Streaming byte accumulator for OSynaptic-RX (C89) */
#include "osrx_parser.h"

void osrx_parser_init(OSRXParser *p, osrx_frame_fn cb, void *ctx)
{
    int i;
    if (!p) { return; }
    for (i = 0; i < OSRX_PACKET_MAX; ++i) { p->buf[i] = 0u; }
    p->len      = 0;
    p->on_frame = cb;
    p->ctx      = ctx;
}

void osrx_parser_reset(OSRXParser *p)
{
    if (!p) { return; }
    p->len = 0;
}

int osrx_feed_byte(OSRXParser *p, osrx_u8 b)
{
    if (!p) { return 0; }
    if (p->len >= OSRX_PACKET_MAX) {
        p->len = 0;   /* overflow: drop and restart */
        return 0;
    }
    p->buf[p->len++] = b;
    return 1;
}

int osrx_feed_done(OSRXParser *p)
{
    osrx_packet_meta  meta;
    osrx_sensor_field field;
    int               ok_struct;
    int               ok_sensor;

    if (!p) { return 0; }

    ok_struct = osrx_packet_decode(p->buf, p->len, &meta);
    if (ok_struct && p->on_frame) {
        ok_sensor = 0;
        if (meta.crc8_ok && meta.crc16_ok &&
            meta.cmd == (osrx_u8)OSRX_CMD_DATA_FULL) {
            ok_sensor = osrx_sensor_unpack(
                p->buf + meta.body_off,
                meta.body_len,
                &field);
        }
        p->on_frame(
            &meta,
            ok_sensor ? &field : (const osrx_sensor_field *)0,
            p->buf,
            p->len,
            p->ctx);
    }

    p->len = 0;
    return ok_struct;
}

int osrx_feed_bytes(OSRXParser *p, const osrx_u8 *data, int len)
{
    int i;
    if (!p || !data || len <= 0) { return 0; }
    for (i = 0; i < len; ++i) {
        osrx_feed_byte(p, data[i]);
    }
    return osrx_feed_done(p);
}
