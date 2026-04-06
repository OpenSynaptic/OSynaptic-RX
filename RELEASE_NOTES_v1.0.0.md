# OSynaptic-RX v1.0.0 — First Stable Release

We are pleased to announce the **first stable release** of OSynaptic-RX — a RX-only, pure C89 packet decoder for 8-bit MCUs that decodes OpenSynaptic FULL wire frames from any serial transport (UART / UDP / LoRa / RS-485 / SPI) into validated sensor readings.

---

## What Is OSynaptic-RX?

OSynaptic-RX is the minimal-footprint MCU-side receiver of the OpenSynaptic sensor telemetry stack. A single `#include <OSynaptic-RX.h>` gives your sketch:

- **Streaming parser** — feed bytes one at a time from a UART ISR or polling loop; the callback fires once a complete frame is assembled.
- **Direct frame API** — for transports that deliver complete frames (UDP, LoRa, SPI with CS), call `osrx_sensor_recv()` on the raw buffer directly — no parser state needed.
- **Pure C89** — compiles clean on avr-gcc, SDCC, IAR, MPLAB XC8, and every host toolchain with `-std=c89 -pedantic -Wall -Wextra -Werror`.
- **No heap** — zero `malloc`/`free` calls; the only state is the `OSRXParser` struct you declare at file scope.
- **Integer-only arithmetic** — no `float` required; values are decoded to a fixed-point scaled integer.
- **Dual CRC validation** — CRC-8/SMBUS over body + CRC-16/CCITT-FALSE over full frame.

---

## Install

**Arduino IDE:** `Sketch > Include Library > Add .ZIP Library…` → select `OSynaptic-RX.zip`

**Manual:** copy the `OSynaptic-RX` folder into `Documents/Arduino/libraries/`

**CMake (native build / unit tests):**

```powershell
cmake -B build -DCMAKE_BUILD_TYPE=MinSizeRel
cmake --build build
ctest --test-dir build --output-on-failure
```

---

## Two Decode Paths

### 1. Streaming parser (UART / RS-485)

Use when transport delivers a byte stream with idle-gap frame boundaries.

```cpp
#include <OSynaptic-RX.h>

static OSRXParser g_parser;

static void on_frame(const osrx_packet_meta  *meta,
                     const osrx_sensor_field *field,
                     const osrx_u8 *, int, void *)
{
    if (!meta->crc8_ok || !meta->crc16_ok || !field) return;
    /* field->sensor_id, field->unit, field->scaled */
}

void setup() {
    Serial.begin(9600);
    osrx_parser_init(&g_parser, on_frame, NULL);
}

void loop() {
    while (Serial.available())
        osrx_feed_byte(&g_parser, (osrx_u8)Serial.read());
    if (idle_gap_detected())
        osrx_feed_done(&g_parser);
}
```

- Stack peak in callback: ~55 B (AVR)
- `OSRXParser` RAM: **102 B** (file-scope declaration)

### 2. Direct frame API (UDP / LoRa / SPI)

Use when transport delivers a complete frame per receive event. No parser state required.

```c
uint8_t buf[OSRX_PACKET_MAX];
int     len = udp.read((char*)buf, sizeof(buf));

osrx_packet_meta  meta;
osrx_sensor_field field;

if (osrx_sensor_recv(buf, len, &meta, &field)) {
    /* field.scaled / OSRX_VALUE_SCALE = real value */
}
```

- Zero extra RAM beyond the two structs on the stack (~41 B AVR)
- Compile with `OSRX_NO_PARSER=1` to save an additional 102 B RAM + 316 B Flash

---

## Wire Format

```
[cmd:1][route:1][aid:4BE][tid:1][ts:6BE][sid|unit|b62][crc8:1][crc16:2]
```

- `cmd = 63` (0x3F) — DATA_FULL plaintext frame
- CRC-8/SMBUS over body (`sid|unit|b62value` field only)
- CRC-16/CCITT-FALSE over full frame (header + body + CRC-8)
- Base62 value encoding with scale factor 10000 (configurable via `OSRX_VALUE_SCALE`)

---

## Flash / RAM Profiles (AVR, `-Os`)

| Profile | Flash | RAM (`OSRXParser`) |
|---------|-------|--------------------|
| Full defaults | ~616 B | 102 B |
| No parser (`OSRX_NO_PARSER=1`) | ~442 B | 0 B |
| No CRC + no parser | ~321 B | 0 B |
| All switches off (minimum) | ~310 B | 0 B |

See [docs/05-flash-optimization.md](docs/05-flash-optimization.md) for a complete profile matrix.

---

## Flash Optimization Switches

| Macro | Default | Flash saved (AVR) | Notes |
|-------|---------|------------------|-------|
| `OSRX_NO_PARSER` | `0` | ~174 B + 102 B RAM | Safe for UDP / LoRa / SPI |
| `OSRX_NO_TIMESTAMP` | `0` | ~20 B | Drops `ts_sec` from struct |
| `OSRX_VALIDATE_CRC8` | `1` | ~80 B (set to 0) | Only on trusted link |
| `OSRX_VALIDATE_CRC16` | `1` | ~90 B (set to 0) | Not recommended |

---

## Supported Architectures

All architectures with a C89-capable compiler and ≥ 128 B RAM (≥ 256 B recommended for streaming parser) / ≥ 2 KB Flash.

Tested: AVR (Uno, Nano, Mega, ATtiny), ESP8266, ESP32, STM32, RP2040, Cortex-M, x86-64.

---

## Examples Included

| Sketch | Board | Transport | API |
|--------|-------|-----------|-----|
| `BasicRX` | Uno / Nano | UART | Streaming parser + Serial |
| `BareMetalUARTRX` | ATmega328P | UART bare-metal registers | Streaming parser, no Serial class |
| `ESP32UdpRX` | ESP32 | WiFi UDP | Parser (`osrx_feed_bytes`) |
| `LoRaRX` | ESP32 / Uno + SX1276 | LoRa | Direct (`osrx_sensor_recv`) |
| `MultiSensorRX` | Any | UART | Parser + AID/sensor_id dispatch |

---

## Known Limitations

- RX / decode only — no transmit path. Use [OSynaptic-TX](../OSynaptic-TX) on the sender side.
- `cmd = 64` (DATA_FULL_SEC, XOR-encrypted) frames are **not** decrypted; the library discards them as non-sensor frames. Encryption is a hub-to-hub feature in the OpenSynaptic protocol; FX sensor nodes always send `cmd = 63`.
- Single frame per parser invocation — concurrent frames from multiple interleaved byte streams require one `OSRXParser` instance per stream.

---

## Changelog

### v1.0.0 (2026-04-07)

- Initial release
- Streaming parser: `OSRXParser`, `osrx_parser_init`, `osrx_feed_byte`, `osrx_feed_done`, `osrx_feed_bytes`, `osrx_parser_reset`
- Packet decoder: `osrx_packet_decode` → `osrx_packet_meta`
- Sensor unpacker: `osrx_sensor_unpack`, `osrx_sensor_recv` → `osrx_sensor_field`
- Base62 decoder: `osrx_b62_decode(s, len, ok)` — length-bounded, no NUL required
- CRC-8/SMBUS + CRC-16/CCITT-FALSE bit-loop implementations (no lookup table; 0 B RAM)
- Flash switches: `OSRX_NO_PARSER`, `OSRX_NO_TIMESTAMP`, `OSRX_VALIDATE_CRC8`, `OSRX_VALIDATE_CRC16`
- Arduino library structure (`src/` flat layout, `OSynaptic-RX.h` wrapper with `OSRX_NO_PARSER` guard)
- CMake build defaults to `MinSizeRel` (`-Os`); verified on x86-64 (all targets, zero warnings)
- Unit tests: 39/39 pass (`tests/test_parse.c` via CTest)
- Five Arduino examples: BasicRX, BareMetalUARTRX, ESP32UdpRX, LoRaRX, MultiSensorRX
- Documentation: five Markdown docs in `docs/` aligned with OSynaptic-TX style
