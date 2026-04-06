# 02 — API Reference

Complete reference for all OSynaptic-RX decoding APIs. All functions are C89-compatible, allocation-free, and safe for use on 8-bit MCUs.

---

## Compile-time Configuration (`osrx_config.h`)

Override any constant before including any OSynaptic-RX header, either via your build system (`-DOSRX_ID_MAX=12`) or a project-local header.

| Macro | Default | Description |
|-------|---------|-------------|
| `OSRX_ID_MAX` | `9` | Max sensor ID length **including NUL byte**. `"T1\0"` = 3. |
| `OSRX_UNIT_MAX` | `9` | Max unit string length **including NUL byte**. `"Cel\0"` = 4. |
| `OSRX_B62_MAX` | `14` | Max Base62 string length **including NUL**. 32-bit signed int encodes to ≤ 7 chars. Must match TX sender. |
| `OSRX_BODY_MAX` | `64` | Max body byte count = `sensor_id + '|' + unit + '|' + b62`. |
| `OSRX_PACKET_MAX` | `96` | Max wire packet byte count = 13 (header) + body + 3 (CRC). Used as `OSRXParser` buffer size. |
| `OSRX_VALUE_SCALE` | `10000L` | Integer scale factor. `field.scaled / OSRX_VALUE_SCALE` = real value. Must match TX sender. |
| `OSRX_CMD_DATA_FULL` | `63` | Wire command byte for plaintext FULL data packets (OpenSynaptic protocol v1). |
| `OSRX_VALIDATE_CRC8` | `1` | Set `0` to skip CRC-8/SMBUS body check (~80 B Flash saved on AVR). |
| `OSRX_VALIDATE_CRC16` | `1` | Set `0` to skip CRC-16/CCITT-FALSE full-frame check. Not recommended. |
| `OSRX_NO_PARSER` | `0` | Set `1` to exclude `osrx_parser.c` entirely (~316 B Flash saved). Safe when transport provides frame boundaries. |
| `OSRX_NO_TIMESTAMP` | `0` | Set `1` to drop `ts_sec` from `osrx_packet_meta` (~20 B Flash, 4 B RAM saved). |

---

## Portable Types (`osrx_types.h`)

| Type | C89 definition | Width |
|------|---------------|-------|
| `osrx_u8` | `unsigned char` | 8-bit |
| `osrx_u16` | `unsigned short` (or `unsigned int` on 8-bit) | 16-bit |
| `osrx_u32` | `unsigned long` | 32-bit |
| `osrx_i32` | `signed long` | 32-bit signed |

---

## Data Structures

### `osrx_packet_meta`

Decoded wire frame header. Filled by `osrx_packet_decode()` and `osrx_sensor_recv()`.

```c
typedef struct osrx_packet_meta {
    osrx_u8  cmd;          /* command byte (63 = DATA_FULL)               */
    osrx_u8  route_count;  /* should be 1 for sensor nodes                */
    osrx_u32 aid;          /* source device ID (big-endian u32 in wire)   */
    osrx_u8  tid;          /* template / transaction ID                   */
#if !OSRX_NO_TIMESTAMP
    osrx_u32 ts_sec;       /* lower 32 bits of the 48-bit wire timestamp  */
#endif
    int      body_off;     /* byte offset of body inside raw packet       */
    int      body_len;     /* body length in bytes                        */
    int      crc8_ok;      /* 1 = CRC-8  of body  verified, 0 = mismatch */
    int      crc16_ok;     /* 1 = CRC-16 of frame verified, 0 = mismatch */
} osrx_packet_meta;
```

**RAM (AVR)**: 1+1+4+1+4+2+2+2+2 = **19 B** (default). With `OSRX_NO_TIMESTAMP=1`: **15 B**.

### `osrx_sensor_field`

Decoded sensor payload. Filled by `osrx_sensor_unpack()` and `osrx_sensor_recv()`.

```c
typedef struct osrx_sensor_field {
    char     sensor_id[OSRX_ID_MAX];    /* null-terminated sensor ID       */
    char     unit[OSRX_UNIT_MAX];       /* null-terminated unit string     */
    osrx_i32 scaled;                    /* value × OSRX_VALUE_SCALE        */
} osrx_sensor_field;
```

**RAM (AVR, defaults)**: 9+9+4 = **22 B**.

To recover the floating-point equivalent as two integers (whole + fractional):

```c
long whole = (long)(field.scaled / OSRX_VALUE_SCALE);
long frac  = (long)(field.scaled % OSRX_VALUE_SCALE);
if (frac < 0) frac = -frac;
/* e.g. whole=-21 frac=5000 for -21.5000 */
```

### `OSRXParser`

Streaming frame accumulator (only available when `OSRX_NO_PARSER=0`).

```c
typedef struct OSRXParser {
    osrx_u8     buf[OSRX_PACKET_MAX];   /* frame accumulation buffer       */
    int         len;                     /* bytes accumulated               */
    osrx_rx_cb  cb;                      /* frame-complete callback         */
    void       *ctx;                     /* user context pointer            */
} OSRXParser;
```

**RAM (AVR, defaults)**: 96+2+2+2 = **102 B**.

The callback type:

```c
typedef void (*osrx_rx_cb)(
    const osrx_packet_meta  *meta,
    const osrx_sensor_field *field,  /* NULL if body is not a sensor frame */
    const osrx_u8           *raw,
    int                      raw_len,
    void                    *ctx
);
```

---

## Streaming Parser API (`osrx_parser.h`)

Available when `OSRX_NO_PARSER=0` (default).

### `osrx_parser_init()`

```c
void osrx_parser_init(OSRXParser *p, osrx_rx_cb cb, void *ctx);
```

Initialise a parser instance. Safe to call multiple times; resets accumulated bytes.

| Parameter | Description |
|-----------|-------------|
| `p` | Pointer to a caller-allocated `OSRXParser`. Must not be NULL. Declare at file scope to avoid 102 B stack cost. |
| `cb` | Frame-complete callback. Called from within `osrx_feed_done()`. Must not be NULL. |
| `ctx` | Arbitrary user pointer passed to every callback invocation. May be NULL. |

**Stack usage (AVR)**: ~6 B (no local variables beyond pointer arithmetic).

---

### `osrx_feed_byte()`

```c
int osrx_feed_byte(OSRXParser *p, osrx_u8 byte);
```

Append one byte to the accumulation buffer.

**Returns**: `1` on success; `0` if the buffer is full (`buf[OSRX_PACKET_MAX-1]` would overflow) — the parser is reset automatically.

**Stack usage (AVR)**: ~4 B.

---

### `osrx_feed_done()`

```c
int osrx_feed_done(OSRXParser *p);
```

Signal that a complete frame has been accumulated. The parser:

1. Calls `osrx_packet_decode()` on the buffered bytes.
2. If successful, calls `osrx_sensor_unpack()` on the body slice.
3. Fires the callback with `meta`, `field` (or NULL), `raw`, `raw_len`, and `ctx`.
4. Resets the accumulation buffer.

**Returns**: `1` if the packet was structurally valid (header decoded cleanly); `0` on malformed packet or empty buffer. Note: a structurally valid packet may still have `crc8_ok=0` or `crc16_ok=0` — check these fields inside the callback.

**Stack usage (AVR)**: ~55 B (includes `osrx_packet_meta` = 19 B + `osrx_sensor_field` = 22 B on the callback stack).

> **Transport-agnostic design**: `osrx_feed_done()` is the frame-boundary signal. For UART, call it after an idle gap; for UDP, call it after `recvfrom`; for SPI, call it in the CS-rising ISR. The parser is completely transport-independent.

---

### `osrx_feed_bytes()`

```c
int osrx_feed_bytes(OSRXParser *p, const osrx_u8 *data, int len);
```

Convenience wrapper: appends all bytes then calls `osrx_feed_done()`.

Equivalent to:
```c
int i;
for (i = 0; i < len; ++i) osrx_feed_byte(p, data[i]);
osrx_feed_done(p);
```

**Returns**: the return value of `osrx_feed_done()`.

---

### `osrx_parser_reset()`

```c
void osrx_parser_reset(OSRXParser *p);
```

Discard all accumulated bytes without parsing. The callback is **not** fired.

Use to recover from a known-corrupt state (e.g. UART framing error detected externally).

---

## Packet Decoder API (`osrx_packet.h`)

### `osrx_packet_decode()`

```c
int osrx_packet_decode(
    const osrx_u8    *packet,
    int               packet_len,
    osrx_packet_meta *out_meta
);
```

Decode the 13-byte wire header and validate CRCs. Does not copy the body; sets `out_meta->body_off` and `out_meta->body_len` to point into the caller's buffer.

**Parameters**:

| Parameter | Description |
|-----------|-------------|
| `packet` | Pointer to the complete wire frame. Must not be NULL. |
| `packet_len` | Total frame length in bytes. Must be ≥ 16. |
| `out_meta` | Output struct. Must not be NULL. |

**Returns**: `1` on structural success (regardless of CRC outcome); `0` if:
- `packet` or `out_meta` is NULL
- `packet_len < 16`
- Body length would exceed `OSRX_BODY_MAX`

**Stack usage (AVR)**: ~12 B (4 B local ints + CRC locals when enabled).

**CRC result fields** (only meaningful when `OSRX_VALIDATE_CRC8/16` are enabled):

| Field | Meaning |
|-------|---------|
| `crc8_ok = 1` | CRC-8/SMBUS of body matches wire value |
| `crc8_ok = 0` | Body corrupted |
| `crc16_ok = 1` | CRC-16/CCITT-FALSE of full frame matches |
| `crc16_ok = 0` | Any byte in the frame was corrupted |

---

## Sensor Field API (`osrx_sensor.h`)

### `osrx_sensor_unpack()`

```c
int osrx_sensor_unpack(
    const osrx_u8    *body,
    int               body_len,
    osrx_sensor_field *out
);
```

Parse a body slice of the form `"sensor_id|unit|b62value"` into `out`.

**Parameters**:

| Parameter | Description |
|-----------|-------------|
| `body` | Pointer to body bytes (not NUL-terminated). Typically `packet + meta.body_off`. |
| `body_len` | Length of body in bytes. Must be > 0. |
| `out` | Output struct. Must not be NULL. |

**Returns**: `1` on success; `0` if:
- `body` is NULL or `body_len ≤ 0`
- Either `'|'` separator is missing or in a position that yields an empty sub-field
- `sensor_id` length ≥ `OSRX_ID_MAX`
- `unit` length ≥ `OSRX_UNIT_MAX`
- Base62 string length ≥ `OSRX_B62_MAX` or contains an invalid character

**Stack usage (AVR)**: ~16 B (4 int locals + no copy buffer; decodes b62 directly from `body`).

---

### `osrx_sensor_recv()`

```c
int osrx_sensor_recv(
    const osrx_u8    *packet,
    int               packet_len,
    osrx_packet_meta *out_meta,
    osrx_sensor_field *out_field
);
```

All-in-one: decode header, validate CRCs, parse body.

**Returns**: `1` if packet is structurally valid, both CRCs pass, and body is a valid sensor frame; `0` otherwise.

**Stack usage (AVR)**: ~12 B (delegates to `osrx_packet_decode` then `osrx_sensor_unpack`; both on the same call stack).

**Typical use (no parser)**:

```c
osrx_packet_meta  meta;
osrx_sensor_field field;

if (osrx_sensor_recv(buf, len, &meta, &field)) {
    long whole = (long)(field.scaled / OSRX_VALUE_SCALE);
    long frac  = (long)(field.scaled % OSRX_VALUE_SCALE);
    if (frac < 0) frac = -frac;
    printf("%s: %ld.%04ld %s\n", field.sensor_id, whole, frac, field.unit);
}
```

---

## Base62 Decoder (`osrx_b62.h`)

### `osrx_b62_decode()`

```c
osrx_i32 osrx_b62_decode(const char *s, int len, int *ok);
```

Decode a length-bounded Base62 string to a 32-bit signed integer.

**Parameters**:

| Parameter | Description |
|-----------|-------------|
| `s` | Pointer to Base62 characters. Need **not** be NUL-terminated. |
| `len` | Number of characters to decode. Pass ≤ 0 for an error result. |
| `ok` | Set to `1` on success, `0` on failure. May be NULL if the return value is sufficient. |

**Alphabet**: `0-9` (indices 0–9), `a-z` (indices 10–35), `A-Z` (indices 36–61). Negative values use a leading `'-'` character (counted in `len`).

**Returns**: decoded `osrx_i32` on success; `0` on any error:
- `s` is NULL
- `len ≤ 0`
- An invalid character (not in alphabet and not `'-'`) is encountered

**Stack usage (AVR)**: ~14 B.

---

## CRC Primitives (`osrx_crc.h`)

These functions are used internally by `osrx_packet_decode()`. They are exposed for integration testing and custom use.

### `osrx_crc8()`

```c
osrx_u8 osrx_crc8(const osrx_u8 *data, int len, osrx_u8 poly, osrx_u8 init);
```

Bit-loop CRC-8 implementation (no lookup table; 86 B Flash on AVR at `-Os`).

For OSynaptic wire frames: `poly=0x07`, `init=0x00`.

### `osrx_crc16()`

```c
osrx_u16 osrx_crc16(const osrx_u8 *data, int len, osrx_u16 poly, osrx_u16 init);
```

Bit-loop CRC-16/CCITT-FALSE implementation (72 B Flash on AVR at `-Os`).

For OSynaptic wire frames: `poly=0x1021`, `init=0xFFFF`.

> **Why bit-loop instead of lookup table**: a CRC-16 lookup table costs 512 B RAM on AVR — 25% of ATmega328P's SRAM. The bit-loop uses 0 B RAM at the cost of ~8% more Flash. On targets with ≥ 2 KB RAM, a lookup table would give faster execution but is not available as a built-in option. If needed, replace the CRC functions using the `OSRX_VALIDATE_CRC8=0` / `OSRX_VALIDATE_CRC16=0` guards and supply your own.
