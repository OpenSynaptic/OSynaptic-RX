#ifndef OSRX_PARSER_H
#define OSRX_PARSER_H

/*
 * osrx_parser.h -- Streaming byte-accumulator for OSynaptic-RX (C89).
 *
 * API:
 *   1. osrx_parser_init()   -- initialise, register callback
 *   2. osrx_feed_byte()     -- push one received byte (UART ISR / polling)
 *   3. osrx_feed_done()     -- signal end-of-frame; fires callback
 *   4. osrx_feed_bytes()    -- convenience: feed + done in one call
 *   5. osrx_parser_reset()  -- discard accumulated bytes without parsing
 */

#include "osrx_types.h"
#include "osrx_config.h"
#include "osrx_packet.h"
#include "osrx_sensor.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*osrx_frame_fn)(
    const osrx_packet_meta  *meta,
    const osrx_sensor_field *field,   /* NULL when body parse failed */
    const osrx_u8           *raw,
    int                      raw_len,
    void                    *ctx
);

typedef struct {
    osrx_u8       buf[OSRX_PACKET_MAX];
    int           len;
    osrx_frame_fn on_frame;
    void         *ctx;
} OSRXParser;

void osrx_parser_init (OSRXParser *p, osrx_frame_fn cb, void *ctx);
void osrx_parser_reset(OSRXParser *p);
int  osrx_feed_byte  (OSRXParser *p, osrx_u8 b);
int  osrx_feed_done  (OSRXParser *p);
int  osrx_feed_bytes (OSRXParser *p, const osrx_u8 *data, int len);

#ifdef __cplusplus
}
#endif

#endif /* OSRX_PARSER_H */
