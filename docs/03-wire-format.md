# 03 — Wire Format

This document describes the OpenSynaptic v1 wire format from the **receiver's perspective**: how to locate fields, verify integrity, and decode sensor data from a raw byte buffer.

For the sender's perspective (field construction, Base62 encoding of values) see the OSynaptic-TX documentation.

---

## 1. Frame Layout

A complete OpenSynaptic v1 frame is a sequence of bytes structured as follows:

| Offset | Length | Field | Encoding | Description |
|--------|--------|-------|----------|-------------|
| 0 | 1 | `cmd` | uint8 | Command byte. `63` = `DATA_FULL` (plaintext). `64` = `DATA_FULL_SEC` (encrypted — **not decoded by RX**). |
| 1 | 1 | `route_count` | uint8 | Hop count / route depth. Typically `1` for direct sensor. |
| 2 | 4 | `aid` | uint32 big-endian | Source device address / ID. |
| 6 | 1 | `tid` | uint8 | Template / transaction ID. Application-defined. |
| 7 | 6 | `timestamp` | 48-bit big-endian | Epoch seconds (lower 32 bits exposed as `ts_sec`). |
| 13 | _B_ | `body` | ASCII | Payload. Length _B_ = total_len − 16. |
| 13+_B_ | 1 | `crc8` | uint8 | CRC-8/SMBUS of body bytes only. |
| 14+_B_ | 2 | `crc16` | uint16 big-endian | CRC-16/CCITT-FALSE of all bytes from offset 0 to (14+_B_−1) inclusive. |

**Minimum frame length**: 13 (header) + 3 (min body "X\|Y\|0") + 3 (CRC bytes) = **19 bytes**.

**Maximum frame length**: `OSRX_PACKET_MAX` (default 96).

---

## 2. Command Byte

| Value | Constant | Description |
|-------|----------|-------------|
| 63 | `OSRX_CMD_DATA_FULL` | Plaintext FULL data frame. OSynaptic-RX decodes this. |
| 64 | `OSRX_CMD_DATA_FULL_SEC` | XOR-encrypted FULL data frame. Session key derived from SHA-256. OSynaptic-RX does **not** decrypt this; discard or forward. |

> **OSynaptic-FX always sends `cmd=63`**. Encryption in the OpenSynaptic protocol is a mutual-negotiation feature negotiated between two OpenSynaptic hub nodes; FX sensor devices always send plaintext.

---

## 3. Source Address (`aid`)

Four bytes at offsets 2–5, big-endian:

```
aid = (uint32_t)packet[2] << 24
    | (uint32_t)packet[3] << 16
    | (uint32_t)packet[4] <<  8
    | (uint32_t)packet[5];
```

The `aid` identifies the transmitting device. Multiple sensor nodes may share the same frame format; the receiving application should use `aid` to index a device registry.

---

## 4. Timestamp (`timestamp`)

Six bytes at offsets 7–12, big-endian. The full value is a 48-bit epoch second counter. OSynaptic-RX exposes only the lower 32 bits:

```c
#if !OSRX_NO_TIMESTAMP
meta.ts_sec = ((osrx_u32)packet[9]  << 24)
            | ((osrx_u32)packet[10] << 16)
            | ((osrx_u32)packet[11] <<  8)
            |  (osrx_u32)packet[12];
#endif
```

The 48-bit full counter wraps every ~8.9 million years at 1 Hz.

> If `OSRX_NO_TIMESTAMP=1`, bytes 7–12 are read for structural purposes (body offset calculation) but their content is **not** stored in `osrx_packet_meta`.

---

## 5. Body Specification

The body is arbitrary ASCII content. For sensor data (the only body type in OSynaptic v1) the format is:

```
body ::= sensor_id "|" unit "|" value
```

| Sub-field | Max length | Example |
|-----------|-----------|---------|
| `sensor_id` | `OSRX_ID_MAX - 1` = 8 chars | `"TempIn"`, `"T1"`, `"HDTY1"` |
| `unit` | `OSRX_UNIT_MAX - 1` = 8 chars | `"Cel"`, `"%RH"`, `"hPa"` |
| `value` | `OSRX_B62_MAX - 1` = 13 chars | `"5cv"` (Base62 for 21.5°C × 10000) |

### Body Parsing Algorithm (used internally by `osrx_sensor_unpack`)

1. Scan body bytes from index `0` for the first `'|'`; record position `p1`.
2. Scan body bytes from index `p1+1` for the second `'|'`; record position `p2`.
3. `sensor_id` = body[0 .. p1-1], length = `p1`.
4. `unit` = body[p1+1 .. p2-1], length = `p2 - p1 - 1`.
5. `value` = body[p2+1 .. body_len-1], length = `body_len - p2 - 1`.
6. Bounds-check each sub-field length. Call `osrx_b62_decode(body+p2+1, seg_len, &ok)`.

---

## 6. Base62 Value Encoding

### Alphabet

| Index range | Characters |
|-------------|-----------|
| 0–9 | `'0'` to `'9'` |
| 10–35 | `'a'` to `'z'` |
| 36–61 | `'A'` to `'Z'` |

A leading `'-'` character encodes a negative value.

### Decode algorithm

$$
v = \sum_{i=0}^{n-1} \, d_i \times 62^{(n-1-i)}
$$

where $d_i$ is the index of character $i$ in the Base62 alphabet, and $n$ is the string length (not counting a leading `'-'`). If a `'-'` was present, negate the result.

#### Worked example: decode `"5cv"` (sensor value 215000 = 21.5 × 10000)

| Char | Index |
|------|-------|
| `'5'` | 5 |
| `'c'` | 12 |
| `'v'` | 31 |

$$v = 5 \times 62^2 + 12 \times 62^1 + 31 \times 62^0 = 19220 + 744 + 31 = 20995$$

Wait — let's verify from source:

```c
/* osrx_b62.c reference */
/* "5cv" in alphabet:
 *   '5' -> 5
 *   'c' -> 12  ('a'=10, 'b'=11, 'c'=12)
 *   'v' -> 31  ('a'=10 + 'v'-'a' = 21) = 31
 *   v = 5*3844 + 12*62 + 31 = 19220 + 744 + 31 = 19995 */
```

> **Note**: the wire value is always `real_value × OSRX_VALUE_SCALE`. For 21.5 °C and `OSRX_VALUE_SCALE=10000`: stored = 215000. The Base62 encoding of 215000 is a 4-character string, not 3.

#### C decode-loop

```c
osrx_i32 osrx_b62_decode(const char *s, int len, int *ok)
{
    static const char *alph = "0123456789abcdefghijklmnopqrstuvwxyz"
                               "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    int sign = 1;
    const char *p   = s;
    const char *end = s + len;
    osrx_i32 v = 0;

    if (!s || len <= 0) { if (ok) *ok = 0; return 0; }
    if (*p == '-') { sign = -1; ++p; }

    while (p < end) {
        const char *q = /* strchr(alph, *p) */ ...;
        if (!q) { if (ok) *ok = 0; return 0; }
        v = v * 62 + (osrx_i32)(q - alph);
        ++p;
    }
    if (ok) *ok = 1;
    return sign * v;
}
```

---

## 7. CRC Specifications

### CRC-8 / SMBUS

| Parameter | Value |
|-----------|-------|
| Width | 8 bits |
| Polynomial | 0x07 (`x⁸ + x² + x + 1`) |
| Init | 0x00 |
| RefIn | false |
| RefOut | false |
| XorOut | 0x00 |
| Coverage | Body bytes only (field 5 in the frame layout) |
| Wire position | Frame offset 13 + body_len (1 byte) |

**Receiver check**:

```c
if (OSRX_VALIDATE_CRC8) {
    expected = osrx_crc8(packet + 13, body_len, 0x07, 0x00);
    meta->crc8_ok = (expected == packet[13 + body_len]);
}
```

### CRC-16 / CCITT-FALSE

| Parameter | Value |
|-----------|-------|
| Width | 16 bits |
| Polynomial | 0x1021 (`x¹⁶ + x¹² + x⁵ + 1`) |
| Init | 0xFFFF |
| RefIn | false |
| RefOut | false |
| XorOut | 0x0000 |
| Coverage | All bytes from offset 0 to (14 + body_len − 1) inclusive; **does not cover the CRC-16 field itself** |
| Wire position | Frame bytes [14+body_len] and [15+body_len] (big-endian) |

**Receiver check**:

```c
if (OSRX_VALIDATE_CRC16) {
    int crc_pos   = 14 + body_len;
    expected16    = osrx_crc16(packet, crc_pos, 0x1021, 0xFFFF);
    wire16        = ((osrx_u16)packet[crc_pos] << 8) | packet[crc_pos + 1];
    meta->crc16_ok = (expected16 == wire16);
}
```

### Bit-loop implementation (used by `osrx_crc.h`)

```c
osrx_u8 osrx_crc8(const osrx_u8 *d, int len, osrx_u8 poly, osrx_u8 init)
{
    osrx_u8 crc = init;
    int i, b;
    for (i = 0; i < len; ++i) {
        crc ^= d[i];
        for (b = 0; b < 8; ++b)
            crc = (crc & 0x80) ? (crc << 1) ^ poly : (crc << 1);
    }
    return crc;
}
```

---

## 8. Complete Decode Flow

```
raw_bytes[0..N]
        │
        ▼
  [ osrx_packet_decode() ]
        │  ├── offset 0: cmd, route_count
        │  ├── offsets 2–5: aid (BE u32)
        │  ├── offset 6: tid
        │  ├── offsets 7–12: timestamp (BE 48-bit)
        │  ├── body = raw[13 .. 13+body_len-1]
        │  ├── CRC-8  verified over body
        │  └── CRC-16 verified over raw[0 .. 14+body_len-1]
        │
        ▼
  osrx_packet_meta (cmd, aid, tid, ts_sec, body_off, body_len, crc*_ok)
        │
        ▼
  [ osrx_sensor_unpack(raw + meta.body_off, meta.body_len, &field) ]
        │  ├── locate first '|' → sensor_id
        │  ├── locate second '|' → unit
        │  └── osrx_b62_decode(value_segment, seg_len, &ok)
        │
        ▼
  osrx_sensor_field (sensor_id[], unit[], scaled)
```

---

## 9. Hex Dump Example

Frame for sensor `T1`, unit `Cel`, value `21.5` (scaled = 215000):

```
Offset  Hex   Dec   Field
  0     3F     63   cmd = DATA_FULL
  1     01      1   route_count
  2     01             ┐
  3     02             │  aid = 0x01020304 = 16909060
  4     03             │
  5     04             ┘
  6     07      7   tid
  7     00             ┐
  8     00             │
  9     66             │  timestamp (48-bit BE)
 10     A3             │  = 0x0000_66A3_xxxx_xxxx
 11     xx             │
 12     xx             ┘
 13     54    'T'  body[0] sensor_id
 14     31    '1'  body[1]
 15     7C    '|'  body[2] separator 1
 16     43    'C'  body[3] unit
 17     65    'e'  body[4]
 18     6C    'l'  body[5]
 19     7C    '|'  body[6] separator 2
 20     2E    '.'  body[7] value (Base62 of 215000 = "pcG")
 21     XX         ... (remaining value chars)
 XX     YY         crc8  (1 byte, body only)
 XX     YY         ┐
 XX     YY         ┘  crc16 (2 bytes BE, full frame)
```
