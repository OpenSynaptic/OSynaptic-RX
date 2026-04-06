#ifndef OSRX_CONFIG_H
#define OSRX_CONFIG_H

/*
 * OSynaptic-RX  --  Compile-time configuration knobs.
 *
 * Override any of these before including osrx_sensor.h, either in your
 * Makefile/CMake (-DOSRX_ID_MAX=12) or in a project-local header that
 * you include before this one.
 *
 * Defaults match OSynaptic-TX so that a TX node and an RX node share the
 * same wire-level parameters without manual synchronisation.
 */

/* Maximum sensor/node ID length INCLUDING the terminating NUL byte. */
#ifndef OSRX_ID_MAX
#  define OSRX_ID_MAX  9
#endif

/* Maximum unit string length INCLUDING the terminating NUL byte. */
#ifndef OSRX_UNIT_MAX
#  define OSRX_UNIT_MAX 9
#endif

/* Maximum Base62-encoded scalar length INCLUDING the terminating NUL. */
#ifndef OSRX_B62_MAX
#  define OSRX_B62_MAX 14
#endif

/* Maximum body byte count to decode. */
#ifndef OSRX_BODY_MAX
#  define OSRX_BODY_MAX 64
#endif

/* Maximum wire packet byte count. */
#ifndef OSRX_PACKET_MAX
#  define OSRX_PACKET_MAX 96
#endif

/* Integer scale factor: real_value = field.scaled / OSRX_VALUE_SCALE. */
#ifndef OSRX_VALUE_SCALE
#  define OSRX_VALUE_SCALE 10000L
#endif

/* Expected command byte for a FULL data packet. */
#ifndef OSRX_CMD_DATA_FULL
#  define OSRX_CMD_DATA_FULL 63
#endif

/* Set to 0 to skip CRC-8/SMBUS body check (saves ~14 B Flash on AVR). */
#ifndef OSRX_VALIDATE_CRC8
#  define OSRX_VALIDATE_CRC8 1
#endif

/* Set to 0 to skip CRC-16/CCITT-FALSE check (not recommended). */
#ifndef OSRX_VALIDATE_CRC16
#  define OSRX_VALIDATE_CRC16 1
#endif

/* Set to 1 to drop the ts_sec field from osrx_packet_meta (~20 B Flash,
 * 4 B RAM saved on AVR).  Disabled by default. */
#ifndef OSRX_NO_TIMESTAMP
#  define OSRX_NO_TIMESTAMP 0
#endif

#endif /* OSRX_CONFIG_H */
