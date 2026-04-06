#ifndef OSRX_CONFIG_H
#define OSRX_CONFIG_H

/*
 * OSynaptic-RX  --  Compile-time configuration knobs.
 *
 * Override before including any osrx_*.h or in your Makefile/CMake:
 *   -DOSRX_PACKET_MAX=48
 *
 * Defaults target an ATmega328P with 2 KB SRAM.
 */

/* Total maximum received frame size in bytes.
 * Frames larger than this will trigger a buffer-overflow reset.
 * Header (13 B) + body + 3 CRC bytes. Default = 96. */
#ifndef OSRX_PACKET_MAX
#  define OSRX_PACKET_MAX 96
#endif

/* Maximum decoded sensor-id length INCLUDING the terminating NUL.
 * "T1\0" = 3, "Sensor12\0" = 9.  Default = 9. */
#ifndef OSRX_ID_MAX
#  define OSRX_ID_MAX 9
#endif

/* Maximum decoded unit string length INCLUDING the terminating NUL.
 * "Cel\0" = 4, "deg_s\0" = 6.  Default = 9. */
#ifndef OSRX_UNIT_MAX
#  define OSRX_UNIT_MAX 9
#endif

/* Maximum body byte count for storage inside OSRXFrame.
 * Must be <= OSRX_PACKET_MAX - 16. Default = 64. */
#ifndef OSRX_BODY_MAX
#  define OSRX_BODY_MAX 64
#endif

/* Maximum Base62-encoded scalar length INCLUDING the terminating NUL.
 * Longest int32 in base62 + sign = 7 chars + NUL = 8. Default = 14. */
#ifndef OSRX_B62_MAX
#  define OSRX_B62_MAX 14
#endif

/* Validate CRC-8/SMBUS over body on receive.
 * Set to 0 to skip and save ~14 B Flash on very constrained targets. */
#ifndef OSRX_VALIDATE_CRC8
#  define OSRX_VALIDATE_CRC8 1
#endif

/* Validate CRC-16/CCITT-FALSE over full frame on receive.
 * Set to 0 to skip (not recommended; disabling drops corruption detection). */
#ifndef OSRX_VALIDATE_CRC16
#  define OSRX_VALIDATE_CRC16 1
#endif

/* Integer scale factor applied to all sensor values on the TX side.
 * real_value = field.scaled / OSRX_VALUE_SCALE  (e.g. 215000/10000 = 21.5) */
#ifndef OSRX_VALUE_SCALE
#  define OSRX_VALUE_SCALE 10000L
#endif

/* Set to 1 to drop the ts_sec field from osrx_packet_meta (~20 B Flash,
 * 4 B RAM saved on AVR).  disabled by default. */
#ifndef OSRX_NO_TIMESTAMP
#  define OSRX_NO_TIMESTAMP 0
#endif

/* Wire command byte for a FULL data packet (OpenSynaptic protocol v1). */
#ifndef OSRX_CMD_DATA_FULL
#  define OSRX_CMD_DATA_FULL 63
#endif

#endif /* OSRX_CONFIG_H */
