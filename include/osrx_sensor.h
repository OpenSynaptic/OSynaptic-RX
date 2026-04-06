#ifndef OSRX_SENSOR_H
#define OSRX_SENSOR_H

#include "osrx_types.h"
#include "osrx_config.h"
#include "osrx_packet.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Decoded sensor field extracted from a packet body.
 *
 * Body layout: "<sensor_id>|<unit>|<b62(scaled)>"
 * After decode: real_value = scaled / OSRX_VALUE_SCALE
 */
typedef struct osrx_sensor_field {
    char     sensor_id[OSRX_ID_MAX];  /* null-terminated sensor name       */
    char     unit[OSRX_UNIT_MAX];     /* null-terminated unit string       */
    osrx_i32 scaled;                  /* encoded value (divide by scale)   */
} osrx_sensor_field;

/*
 * Unpack a sensor reading from a raw packet body slice.
 *
 * 'body' points to the body bytes inside the raw wire buffer
 * (use out_meta.body_off from osrx_packet_decode to get the pointer).
 * 'body_len' is out_meta.body_len.
 *
 * Returns 1 on success with 'out' filled; 0 on parse error.
 *
 * Extended reading example (integer-only, no float needed):
 *   osrx_i32 whole = field.scaled / OSRX_VALUE_SCALE;
 *   osrx_i32 frac  = field.scaled % OSRX_VALUE_SCALE;   (always >= 0)
 *   printf("%ld.%04ld %s\n", (long)whole, (long)frac, field.unit);
 */
int osrx_sensor_unpack(
    const osrx_u8    *body,
    int               body_len,
    osrx_sensor_field *out
);

/*
 * Convenience: decode header + body in one call.
 *
 * Parses the raw packet, validates CRC, then unpacks the sensor field.
 * Returns 1 on full success; 0 on any error.
 * On CRC failure the function still returns 0 (integrity enforced).
 */
int osrx_sensor_recv(
    const osrx_u8    *packet,
    int               packet_len,
    osrx_packet_meta *out_meta,
    osrx_sensor_field *out_field
);

#ifdef __cplusplus
}
#endif

#endif /* OSRX_SENSOR_H */
