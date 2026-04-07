# OSynaptic-RX

简体中文说明请见 [README.zh.md](README.zh.md)。

**RX-only OpenSynaptic packet decoder for 8-bit MCUs** — decodes OpenSynaptic sensor frames from any serial transport (UART / UDP / LoRa / RS-485 / SPI) into validated integer sensor readings with **pure C89**, **no heap**, and a stack peak as low as **55 bytes** on AVR. Pairs directly with [OSynaptic-TX](../OSynaptic-TX/README.md) sensor nodes and the [OpenSynaptic](../OpenSynaptic/README.md) Python hub.

![C89](https://img.shields.io/badge/C-89-00599C?logo=c&logoColor=white)
![Version](https://img.shields.io/badge/version-1.0.0-2E8B57)
![Arduino](https://img.shields.io/badge/Arduino-Library-00979D?logo=arduino&logoColor=white)
![Status](https://img.shields.io/badge/status-stable-2E8B57)
![License](https://img.shields.io/badge/License-Apache--2.0-blue)

![AVR](https://img.shields.io/badge/AVR-supported-00599C)
![ESP32](https://img.shields.io/badge/ESP32-supported-E7352C?logo=espressif&logoColor=white)
![ESP8266](https://img.shields.io/badge/ESP8266-supported-E7352C?logo=espressif&logoColor=white)
![STM32](https://img.shields.io/badge/STM32-supported-03234B?logo=stmicroelectronics&logoColor=white)
![RP2040](https://img.shields.io/badge/RP2040-supported-B41F47?logo=raspberrypi&logoColor=white)
![Cortex-M](https://img.shields.io/badge/Cortex--M-supported-0091BD)

---

## Quick Reference Tables

| Table | Description |
|-------|-------------|
| [**MCU Config Reference →**](docs/04-mcu-config-reference.md) | AVR, STM32, ESP MCU families — recommended config tier, Flash/RAM budget, `OSRX_PACKET_MAX` sizing, and flash reduction switches |
| [**Flash Optimization →**](docs/05-flash-optimization.md) | Complete profile matrix: 8 switch combinations with measured Flash/RAM values, per-switch guidance, CRC trade-offs |

> New to the library? See [Quick Start](#quick-start) below.
> Deploying on a specific MCU? Use the table links above.

---

## Try In 30 Seconds

```
Arduino IDE → Sketch > Include Library > Add .ZIP Library → select OSynaptic-RX.zip
File > Examples > OSynaptic-RX > BasicRX → Upload
```

Connect an OSynaptic-TX node to the RX board's Serial port. Open Serial Monitor at **9600 baud** — decoded sensor readings appear once per received frame.

---

## Table of Contents

- [**Quick Reference Tables**](#quick-reference-tables) — MCU config + flash optimization (start here)
- [Why OSynaptic-RX](#why-osynaptic-rx)
- [Two Decode Paths](#two-decode-paths)
- [Memory Usage](#memory-usage)
- [MCU Deployment Reference](#mcu-deployment-reference)
- [Transport Selection Guide](#transport-selection-guide)
- [Quick Start](#quick-start)
- [Wire Format](#wire-format)
- [API Reference](#api-reference)
- [Configuration](#configuration-osrx_configh)
- [Examples](#examples)
- [Repository Map](#repository-map)
- [CMake Build](#cmake-build)
- [Test Results](#test-results)
- [Documentation](#documentation)
- [Contributing](#contributing)
- [License](#license)

---

## Why OSynaptic-RX

- **RX-only, no encode code**: Flash cost is a fraction of a full-duplex library.
- **C89 clean**: compiles on every toolchain that targets 8-bit MCUs — avr-gcc, SDCC, IAR, MPLAB XC8.
- **No heap**: zero `malloc`/`free` calls; all state in a stack-or-global `OSRXParser`.
- **No float**: values decoded into fixed-point scaled integers (`field->scaled / OSRX_VALUE_SCALE`).
- **Dual CRC**: CRC-8/SMBUS (body) + CRC-16/CCITT-FALSE (full frame) — bit-loop, 0 B RAM, no lookup table.
- **Spec-compatible**: decodes every frame produced by OSynaptic-TX and the OpenSynaptic Python hub without glue code.

---

## Two Decode Paths

| Path | API | Best for |
|------|-----|---------|
| **Streaming parser** | `osrx_feed_byte()` + `osrx_feed_done()` | UART with idle-gap, USB-CDC |
| **Direct frame decode** | `osrx_sensor_recv()` | UDP, LoRa, SPI (frame boundary from transport) |

The streaming parser uses `OSRXParser` (102 B RAM on AVR). For transports with natural frame boundaries, set `OSRX_NO_PARSER=1` to save 102 B RAM + 316 B Flash and call `osrx_sensor_recv()` directly on the datagram buffer.

---

## Memory Usage

### Streaming parser (UART / RS-485)

| Resource | Usage |
|----------|-------|
| `OSRXParser` static RAM | 102 B (`buf[96]` + len + fn_ptr + ctx_ptr) |
| Stack peak in callback | ~55 B (parse → callback chain) |
| Flash (full defaults, -Os) | ~616 B |

### Direct decode (UDP / LoRa, `OSRX_NO_PARSER=1`)

| Resource | Usage |
|----------|-------|
| `OSRXParser` RAM | **0 B** (parser excluded) |
| Stack peak (`osrx_sensor_recv`) | ~41 B (`osrx_packet_meta` 19 B + `osrx_sensor_field` 22 B) |
| Flash (-Os) | ~442 B |

### Minimum supported MCU (streaming)

| Requirement | Value |
|-------------|-------|
| RAM | ≥ 256 B (102 B parser + application logic) |
| Flash | ≥ 2 KB |
| Example | ATmega88 / ATmega168 |

### Minimum supported MCU (no parser)

| Requirement | Value |
|-------------|-------|
| RAM | ≥ 64 B |
| Flash | ≥ 1 KB |
| Example | ATtiny85 / ATmega48 |

---

## MCU Deployment Reference

### 8-bit AVR family — receiver (gateway / hub / display) deployments

| MCU | Flash | RAM | UART | Parser mode | Config tier | Notes |
|-----|-------|-----|------|-------------|-------------|-------|
| ATtiny85 | 8 KB | 512 B | USI/SW | **No parser** | **Tight** | Disable parser; feed full LoRa/UDP datagram directly. |
| ATmega48 | 4 KB | 512 B | HW UART0 | No parser | **Tight** | Only 512 B total RAM; disable parser to fit. |
| ATmega88 | 8 KB | 1 KB | HW UART0 | Full parser | **Standard** | First AVR comfortable with full parser. |
| ATmega168 | 16 KB | 1 KB | HW UART0 | Full parser | **Standard** | |
| **ATmega328P** | **32 KB** | **2 KB** | **HW UART0** | **Full parser** | **Standard** | **Arduino Uno / Nano baseline. Recommended entry point.** |
| ATmega32U4 | 32 KB | 2.5 KB | HW UART + USB | Full parser | **Standard** | USB-CDC: feed bytes from `SerialUSB.read()`. |
| ATmega2560 | 256 KB | 8 KB | 4× HW UART | Full parser | **Comfort** | Multi-channel: one `OSRXParser` per UART × 4. |
| ATmega4809 | 48 KB | 6 KB | 4× USART | Full parser | **Comfort** | Arduino Nano Every (megaAVR-0); UPDI programming. |

> **Config tiers**: Ultra / Tight / Standard / Comfort. See [docs/04-mcu-config-reference.md](docs/04-mcu-config-reference.md) for macro settings per tier.

### 32-bit targets

| MCU | Flash | RAM | Parser mode | Notes |
|-----|-------|-----|-------------|-------|
| STM32F030F4 | 16 KB | 4 KB | Full parser | Cortex-M0; all APIs fit comfortably. |
| STM32F103C8 | 64 KB | 20 KB | Full parser | Multi-channel receiver; 3× UART available. |
| ESP8266 | 1–4 MB | 80 KB | Full parser | One UDP datagram per `osrx_feed_bytes` call. |
| **ESP32** | **4 MB** | **520 KB** | **Full parser** | **Preferred LAN gateway platform.** |
| RP2040 | 2 MB | 264 KB | Full parser | PIO-based UART; DMA callback into `osrx_feed_bytes`. |

---

## Transport Selection Guide

Choose the transport that matches your hardware. The parser choice follows automatically.

### UART / RS-485 (streaming — use parser)
**Best for**: AVR/STM32 nodes within cable reach of the TX device.

- Feed bytes from the UART ISR or polling loop using `osrx_feed_byte()`.
- Call `osrx_feed_done()` after a 15 ms idle gap (no byte received).
- For distances > 10 m add an RS-485 driver (MAX485 / SN75176); library output is identical.

### WiFi UDP (ESP32 / ESP8266 — no parser needed)
**Best for**: ESP32 / ESP8266 gateway nodes receiving from OSynaptic-TX UDP senders.

- Each UDP datagram = one complete OpenSynaptic frame.
- Call `osrx_feed_bytes(buf, n)` or `osrx_sensor_recv(buf, n, &meta, &field)` per `parsePacket()`.
- No idle-gap detection; no `OSRXParser` state; set `OSRX_NO_PARSER=1`.

### LoRa (SX1276/SX1278 — no parser needed)
**Best for**: outdoor or long-range gateways with LoRa radio modules.

- Each LoRa packet holds one complete frame; call `osrx_sensor_recv()` inside `onReceive`.
- `RSSI` and `SNR` available from `LoRa.packetRssi()` / `LoRa.packetSnr()`.
- See `LoRaRX` example.

### SPI / I²C bridge (no parser needed)
**Best for**: short PCB-to-PCB links where CS / stop-bit marks frame boundary.

- SPI CS-rising ISR → `osrx_feed_done()`.
- I²C stop-condition → `osrx_feed_done()`.

### Multi-channel (ATmega2560 / STM32 — one parser per channel)
**Best for**: hub nodes receiving from multiple independent TX streams simultaneously.

```c
static OSRXParser chan[4];  /* 4 × 102 B = 408 B RAM on ATmega2560 */
/* each UART ISR feeds its own parser */
void uart0_isr(void) { osrx_feed_byte(&chan[0], UDR0); }
```

---

## Quick Start

### Arduino — streaming, UART with idle gap

```cpp
#include <OSynaptic-RX.h>

static OSRXParser parser;

static void on_frame(const osrx_packet_meta  *meta,
                     const osrx_sensor_field *field,
                     const osrx_u8 *, int, void *)
{
    if (!meta->crc8_ok || !meta->crc16_ok) return;   /* discard corrupt */
    if (!field) return;                                /* not a sensor frame */

    /* Integer-only value: real = field->scaled / OSRX_VALUE_SCALE */
    Serial.print(field->sensor_id);  Serial.print(": ");
    Serial.print((long)(field->scaled / OSRX_VALUE_SCALE));
    Serial.print(" ");  Serial.println(field->unit);
}

void setup() {
    Serial.begin(9600);
    osrx_parser_init(&parser, on_frame, nullptr);
}

static unsigned long last_byte = 0;
static bool          got_byte  = false;

void loop() {
    while (Serial.available()) {
        osrx_feed_byte(&parser, (osrx_u8)Serial.read());
        last_byte = millis();  got_byte = true;
    }
    if (got_byte && millis() - last_byte > 15) {  /* 15 ms idle = frame end */
        osrx_feed_done(&parser);
        got_byte = false;
    }
}
```

### ESP32 UDP — one datagram per frame (no parser)

```cpp
#include <OSynaptic-RX.h>

static uint8_t udp_buf[OSRX_PACKET_MAX];

void loop() {
    int n = udp.parsePacket();
    if (n > 0) {
        int len = udp.read((char*)udp_buf,
                           n < OSRX_PACKET_MAX ? n : OSRX_PACKET_MAX);
        osrx_packet_meta  meta;
        osrx_sensor_field field;
        if (osrx_sensor_recv(udp_buf, len, &meta, &field)) {
            /* field.sensor_id, field.unit,
               field.scaled / OSRX_VALUE_SCALE = real value */
        }
    }
}
```

### Native C — CMake / bare-metal / desktop

```c
#include "osrx_sensor.h"

osrx_packet_meta  meta;
osrx_sensor_field field;

if (osrx_sensor_recv(buf, buf_len, &meta, &field)) {
    long whole = (long)(field.scaled / OSRX_VALUE_SCALE);
    long frac  = (long)(field.scaled % OSRX_VALUE_SCALE);
    if (frac < 0) frac = -frac;
    printf("%s: %ld.%04ld %s\n", field.sensor_id, whole, frac, field.unit);
}
```

---

## Wire Format

Every frame follows the OpenSynaptic FULL packet layout (C89 big-endian):

```
[cmd:1][route:1][aid:4BE][tid:1][ts:6BE][sid|unit|b62][crc8:1][crc16:2]
 ──────────────── 13 bytes header ─────────────────── body ────── ──3 CRC─
```

| Field | Size | Description |
|-------|------|-------------|
| `cmd` | 1 B | `0x3F` (63) = DATA_FULL plaintext. `0x40` (64) = encrypted (not decoded by RX). |
| `route` | 1 B | Routing/hop count flags |
| `aid` | 4 B | Source agent ID (big-endian) |
| `tid` | 1 B | Transaction ID (wraps 0–255) |
| `ts` | 6 B | Unix timestamp seconds (48-bit big-endian) |
| body | variable | `sensor_id\|unit\|b62value` |
| `crc8` | 1 B | CRC-8/SMBUS (poly `0x07`, init `0x00`) over body |
| `crc16` | 2 B | CRC-16/CCITT-FALSE (poly `0x1021`, init `0xFFFF`) over full frame |

Minimum frame size: **19 bytes** (2-char sid, 1-char unit, 1-char b62 value).

See [docs/03-wire-format.md](docs/03-wire-format.md) for complete byte-level specification, CRC algorithms, and worked Base62 decode example.

---

## API Reference

### `osrx_sensor.h` — All-in-one decode (recommended)

```c
/* Decode header + body + validate both CRCs. Returns 1 on full success. */
int osrx_sensor_recv(const osrx_u8 *packet, int len,
                     osrx_packet_meta *meta, osrx_sensor_field *field);

/* Parse "sid|unit|b62" body slice -> osrx_sensor_field. */
int osrx_sensor_unpack(const osrx_u8 *body, int body_len,
                       osrx_sensor_field *out);
```

### `osrx_parser.h` — Streaming byte accumulator

| Function | Description |
|---|---|
| `osrx_parser_init(p, cb, ctx)` | Initialise parser; register callback |
| `osrx_feed_byte(p, b)` | Push one byte (returns 0 on overflow/reset) |
| `osrx_feed_done(p)` | Signal frame end; parse + callback; returns 1 if structurally valid |
| `osrx_feed_bytes(p, data, len)` | Feed all bytes + `osrx_feed_done` in one call |
| `osrx_parser_reset(p)` | Discard accumulated bytes without parsing |

### `osrx_packet.h` — Header-only decoder

```c
int osrx_packet_decode(const osrx_u8 *packet, int len, osrx_packet_meta *out);
```

### `osrx_b62.h` — Base62 decoder

```c
osrx_i32 osrx_b62_decode(const char *s, int len, int *ok);
```

`s` need not be NUL-terminated; `len` is the number of significant characters.

### `osrx_crc.h` — CRC primitives

```c
osrx_u8  osrx_crc8 (data, len, poly=0x07,   init=0x00);
osrx_u16 osrx_crc16(data, len, poly=0x1021, init=0xFFFF);
```

---

## Configuration (`osrx_config.h`)

Override via CMake `-D` flags or `#define` before including:

| Macro | Default | Meaning |
|---|---|---|
| `OSRX_PACKET_MAX` | 96 | Max wire frame size in bytes (= `OSRXParser` buffer size) |
| `OSRX_ID_MAX` | 9 | Max sensor ID length + NUL |
| `OSRX_UNIT_MAX` | 9 | Max unit string length + NUL |
| `OSRX_B62_MAX` | 14 | Max Base62 string length + NUL |
| `OSRX_BODY_MAX` | 64 | Max body byte count |
| `OSRX_VALUE_SCALE` | 10000 | Divide `scaled` by this to get real value |
| `OSRX_CMD_DATA_FULL` | 63 | Expected command byte for sensor frames |
| `OSRX_VALIDATE_CRC8` | 1 | Set 0 to skip body CRC check (~80 B Flash saved on AVR) |
| `OSRX_VALIDATE_CRC16` | 1 | Set 0 to skip frame CRC check — not recommended |
| `OSRX_NO_PARSER` | 0 | Set 1 to exclude streaming parser (~316 B Flash + 102 B RAM saved) |
| `OSRX_NO_TIMESTAMP` | 0 | Set 1 to drop `ts_sec` from `osrx_packet_meta` (~20 B Flash, 4 B RAM) |

---

## Examples

| Example | Transport | Parser | Target |
|---------|-----------|--------|--------|
| [BasicRX](examples/BasicRX/BasicRX.ino) | UART (idle-gap) | Streaming | Any Arduino |
| [MultiSensorRX](examples/MultiSensorRX/MultiSensorRX.ino) | UART (idle-gap) | Streaming | Any Arduino, AID+sensor dispatch |
| [ESP32UdpRX](examples/ESP32UdpRX/ESP32UdpRX.ino) | WiFi UDP | `osrx_feed_bytes` | ESP32 / ESP8266 |
| [LoRaRX](examples/LoRaRX/LoRaRX.ino) | LoRa SX1276 | Direct (`osrx_sensor_recv`) | Heltec / TTGO / Uno + shield |
| [BareMetalUARTRX](examples/BareMetalUARTRX/BareMetalUARTRX.ino) | USART0 registers | Streaming, no Serial overhead | ATmega328P |

---

## Repository Map

```
OSynaptic-RX/
├── OSynaptic-RX.h          ← single Arduino include
├── library.properties
├── keywords.txt
├── LICENSE
├── README.md
├── RELEASE_NOTES_v1.0.0.md
├── CONTRIBUTING.md
├── SECURITY.md
├── cmake/
│   └── osrxConfig.cmake.in ← find_package(osrx) template
├── docs/
│   ├── 01-deployment-guide.md      ← hardware targets, transport adapters, buffer sizing
│   ├── 02-api-reference.md         ← complete API reference with stack usage
│   ├── 03-wire-format.md           ← byte-level wire format, CRC algorithms, Base62 decode
│   ├── 04-mcu-config-reference.md  ← Ultra/Tight/Standard/Comfort tiers; per-MCU table
│   └── 05-flash-optimization.md    ← 8 flash profiles; per-switch guidance; bit-loop rationale
├── src/
│   ├── osrx_config.h       ← compile-time knobs
│   ├── osrx_types.h        ← C89 portable typedefs
│   ├── osrx_crc.h/c        ← CRC-8 + CRC-16 (bit-loop, no lookup table)
│   ├── osrx_b62.h/c        ← Base62 decoder (length-bounded, no NUL required)
│   ├── osrx_packet.h/c     ← wire header decoder
│   ├── osrx_sensor.h/c     ← body parser + all-in-one osrx_sensor_recv()
│   ├── osrx_parser.h/c     ← streaming byte accumulator (OSRXParser)
│   └── OSynaptic-RX.h      ← Arduino src-layout entry
├── include/                ← public headers for CMake consumers
├── examples/
│   ├── basic_rx.c                      ← native C (CMake)
│   ├── BasicRX/                        ← Arduino UART streaming
│   ├── MultiSensorRX/                  ← UART, multi-sensor dispatch by AID
│   ├── ESP32UdpRX/                     ← WiFi UDP, ESP32
│   ├── LoRaRX/                         ← LoRa SX1276, no parser
│   └── BareMetalUARTRX/                ← AVR registers, no Serial overhead
├── tests/
│   └── test_parse.c        ← known-answer unit tests (39 assertions)
└── CMakeLists.txt
```

---

## CMake Build

Requires CMake ≥ 3.10 and a C89-capable compiler.

### Build and test

```powershell
cmake -B build                            # defaults to MinSizeRel (-Os)
cmake -B build -DCMAKE_BUILD_TYPE=Debug   # debug build
cmake --build build
ctest --test-dir build --output-on-failure
```

### Install

```powershell
cmake --install build --prefix /usr/local
```

Installed layout:

```
/usr/local/
├── lib/
│   ├── libosrx.a
│   └── cmake/osrx/
│       ├── osrxConfig.cmake
│       ├── osrxConfigVersion.cmake
│       ├── osrxTargets.cmake
│       └── osrxTargets-minsizerel.cmake
└── include/
    └── osrx/
        ├── osrx_config.h
        ├── osrx_types.h
        ├── osrx_crc.h
        ├── osrx_b62.h
        ├── osrx_packet.h
        ├── osrx_sensor.h
        └── osrx_parser.h
```

### Consume with `find_package`

```cmake
# In your project's CMakeLists.txt:
find_package(osrx 1.0 REQUIRED)
target_link_libraries(myapp PRIVATE osrx::osrx)
```

The imported target `osrx::osrx` carries its `target_include_directories` and `target_compile_definitions` through usage requirements — no manual `-I` or `-D` flags needed.

### Flash reduction switches

```powershell
# Tight tier example (ATmega88): no parser, no timestamp
cmake -B build -DOSRX_NO_PARSER=ON -DOSRX_NO_TIMESTAMP=ON

# Ultra tier: all switches off
cmake -B build -DOSRX_NO_PARSER=ON -DOSRX_NO_TIMESTAMP=ON `
               -DOSRX_VALIDATE_CRC8=0 -DOSRX_VALIDATE_CRC16=0
```

---

## Test Results

Run the built-in known-answer test suite to verify the CRC, Base62, and packet decode implementations on your build host:

```sh
cmake -B build -DOSRX_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

| Group | Assertions | What is verified |
|-------|-----------|------------------|
| CRC-8/SMBUS | — | Standard check value `0xF4` for `"123456789"`; single-byte; NULL/zero-length guard |
| CRC-16/CCITT-FALSE | — | Standard check value `0x29B1` for `"123456789"`; two edge bytes; NULL guard |
| Base62 decode | — | Zero; negative; alphabet boundaries; rollover; `INT32_MIN`; NULL pointer and len ≤ 0 guards |
| Frame decode | — | `aid` big-endian; `ts_sec` bytes; body offset; CRC-8 position; CRC-16 big-endian |
| Sensor unpack | — | Valid body; missing `\|`; sub-field too long; invalid b62 char |
| `OSRX_NO_TIMESTAMP` | — | Struct layout correct; field absent when flag set |
| **Total** | **39** | Expected: `39 passed, 0 failed` |

---

## Documentation

Full documentation is in the [`docs/`](docs/) folder:

| File | Contents |
|------|----------|
| [docs/01-deployment-guide.md](docs/01-deployment-guide.md) | Hardware requirements, AVR/32-bit deployment tables, transport adapters, `OSRXParser` sizing |
| [docs/02-api-reference.md](docs/02-api-reference.md) | Complete API reference with parameter tables, return values, and AVR stack usage |
| [docs/03-wire-format.md](docs/03-wire-format.md) | Byte-level wire format (RX perspective), CRC specs, Base62 decode algorithm |
| [docs/04-mcu-config-reference.md](docs/04-mcu-config-reference.md) | Ultra/Tight/Standard/Comfort tiers; per-MCU fit table; Arduino IDE config |
| [docs/05-flash-optimization.md](docs/05-flash-optimization.md) | 8 flash profiles with measured values; per-switch guidance; bit-loop vs lookup-table rationale |

---

## Contributing

Please read [CONTRIBUTING.md](CONTRIBUTING.md) and [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md) before opening a pull request.

---

## License

Apache License 2.0 — see [LICENSE](LICENSE).
