#ifndef OSRX_PARSER_H
#define OSRX_PARSER_H

/*
 * osrx_parser.h -- Streaming byte-accumulator for OSynaptic-RX (C89).
 *
 * API:
 *   1. osrx_parser_init()   -- initialise parser, register callback
 *   2. osrx_feed_byte()     -- push one received byte (UART ISR / polling)
 *   3. osrx_feed_done()     -- signal end-of-frame (UART timeout, UDP end)
 *                              triggers parse and fires callback
 *   4. osrx_feed_bytes()    -- convenience: feed+done in one call
 *   5. osrx_parser_reset()  -- discard accumulated bytes without parsing
 *
 * The frame callback receives both the packet header (always valid on
 * structural success) and the sensor field (NULL when body parse fails
 * or the frame is not a sensor/data packet).
 *
 * The accumulation buffer is fixed-size (OSRX_PACKET_MAX bytes).
 * An overflow silently discards the packet and resets the buffer.
 */

#include "osrx_types.h"
#include "osrx_config.h"
#include "osrx_packet.h"
#include "osrx_sensor.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Frame callback type.
 *
 * Parameters:
 *   meta   -- parsed packet header (always valid when called)
 *   field  -- parsed sensor reading; NULL when body parse failed or
 *             the frame is not a OSRX_CMD_DATA_FULL frame
 *   raw    -- the raw byte buffer as received
 *   raw_len -- number of bytes in 'raw'
 *   ctx    -- user context pointer passed to osrx_parser_init()
 *
 * CRC fidelity: meta->crc8_ok and meta->crc16_ok carry the verification
 * result. The application SHOULD discard readings when either is 0.
 */
typedef void (*osrx_frame_fn)(
    const osrx_packet_meta  *meta,
    const osrx_sensor_field *field,
    const osrx_u8           *raw,
    int                      raw_len,
    void                    *ctx
);

/*
 * Parser state (embed in your device context; no heap required).
 */
typedef struct {
    osrx_u8       buf[OSRX_PACKET_MAX]; /* byte accumulation buffer       */
    int           len;                  /* bytes accumulated so far       */
    osrx_frame_fn on_frame;             /* callback fired when frame done */
    void         *ctx;                  /* forwarded user pointer         */
} OSRXParser;

/* Initialise parser and register frame callback.
 * 'cb' may be NULL (frames are parsed but callback is not invoked). */
void osrx_parser_init (OSRXParser *p, osrx_frame_fn cb, void *ctx);

/* Discard accumulated bytes without parsing. */
void osrx_parser_reset(OSRXParser *p);

/* Push one byte into the accumulator.
 * Returns 1 on success, 0 on buffer overflow (buffer is auto-reset). */
int  osrx_feed_byte  (OSRXParser *p, osrx_u8 b);

/* Signal end-of-frame: parse accumulated bytes, fire callback, then reset.
 * Returns 1 if the frame was structurally valid (even on CRC mismatch). */
int  osrx_feed_done  (OSRXParser *p);

/* Push 'len' bytes then call osrx_feed_done().
 * Returns 1 if the resulting frame was structurally valid. */
int  osrx_feed_bytes (OSRXParser *p, const osrx_u8 *data, int len);

#ifdef __cplusplus
}
#endif

#endif /* OSRX_PARSER_H */
