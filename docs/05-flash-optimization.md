# 05 — Flash Optimization

This document describes OSynaptic-RX's flash reduction switches, measured size profiles, and guidelines for choosing the right trade-off on AVR and other constrained devices.

---

## 1. The Problem

AVR microcontrollers have between 1 KB and 256 KB of Flash but as little as 512 B of RAM. Even though OSynaptic-RX is designed for minimum footprint, the default build (~616 B on AVR at `-Os`) may be too large for devices that are also running application code, sensor drivers, and a display or radio library.

OSynaptic-RX exposes four independent compile-time switches that eliminate whole compilation units or data paths. These are additive: you can enable any combination independently.

---

## 2. Flash Size Profiles

All measurements at `-Os` (`MinSizeRel`), AVR estimate = x86-64 `.text` × 0.55.

| Profile | `OSRX_VALIDATE_CRC8` | `OSRX_VALIDATE_CRC16` | `OSRX_NO_PARSER` | `OSRX_NO_TIMESTAMP` | x86-64 text | AVR est. |
|---------|---------|---------|---------|---------|------------|---------|
| Full defaults | 1 | 1 | 0 | 0 | 1120 B | **~616 B** |
| No timestamp | 1 | 1 | 0 | 1 | 1100 B | **~605 B** |
| No parser | 1 | 1 | 1 | 0 | ~804 B | **~442 B** |
| No parser, no ts | 1 | 1 | 1 | 1 | ~784 B | **~431 B** |
| No CRC-8 | 0 | 1 | 0 | 0 | ~1040 B | **~572 B** |
| No CRC-16 | 1 | 0 | 0 | 0 | ~1050 B | **~578 B** |
| No CRC (both) | 0 | 0 | 0 | 0 | ~960 B | **~528 B** |
| No CRC, no parser | 0 | 0 | 1 | 0 | ~584 B | **~321 B** |
| **Minimum** (all off) | **0** | **0** | **1** | **1** | **~564 B** | **~310 B** |

> The "minimum" profile retains `osrx_sensor_unpack()` + `osrx_b62_decode()` + `osrx_packet_decode()` — enough to decode raw packets with no integrity checking and no streaming. Useful for bootloaders or sensor forwarders.

### Build type effect

| Build type | AVR size (full defaults) | Reduction vs `-O0` |
|------------|--------------------------|-------------------|
| `-O0` (debug) | ~1330 B | — |
| `-O1` | ~740 B | −44% |
| **`-Os` (MinSizeRel)** | **~616 B** | **−54%** |
| `-O2` / `-O3` | ~670 B | −50% (speed, not size) |

> OSynaptic-RX CMakeLists.txt defaults to `MinSizeRel` if no other build type is set. This matches the behaviour of the Arduino IDE for `.cpp` / `.c` files and should be used for all embedded deployments.

---

## 3. Per-Switch Details

### `OSRX_VALIDATE_CRC8`

Default: `1`

When `0`:
- The `osrx_crc8()` function body is compiled out.
- `meta.crc8_ok` is always set to `1` (assume valid).
- Flash savings: **~80 B** on AVR.

**When to disable**: trusted wire (direct dedicated UART between two boards, no cable), or when a hardware CRC module computes CRC-8 before the software library is invoked. Do **not** disable over radio links or multi-drop RS-485.

---

### `OSRX_VALIDATE_CRC16`

Default: `1`

When `0`:
- The `osrx_crc16()` function body is compiled out.
- `meta.crc16_ok` is always set to `1`.
- Flash savings: **~90 B** on AVR.

> CRC-16/CCITT-FALSE covers the entire frame (header + body + CRC-8). If only one CRC can be validated, prefer CRC-16 over CRC-8 because it catches header corruption in addition to body corruption.

**When to disable**: same conditions as CRC-8, but require additional confidence in link layer (e.g. CRC-16 validated by radio hardware below the parser).

---

### `OSRX_NO_PARSER`

Default: `0`

When `1`:
- `osrx_parser.c` / `osrx_parser.h` are excluded from compilation.
- `OSRXParser`, `osrx_parser_init()`, `osrx_feed_byte()`, `osrx_feed_done()`, `osrx_feed_bytes()`, and `osrx_parser_reset()` are all unavailable.
- Flash savings on AVR: **~174 B**; **RAM savings: 102 B** (no `OSRXParser` buffer).

**When to enable**:

| Transport | Parser needed? |
|-----------|---------------|
| UART with idle-gap detection | **Yes — keep parser** |
| USB-CDC with idle-gap | **Yes — keep parser** |
| UDP (one datagram = one frame) | **No — disable parser** |
| SPI with CS (~frame boundary) | **No — disable parser** |
| LoRa with packet-complete IRQ | **No — disable parser** |
| I²C with stop-bit (~frame boundary) | **No — disable parser** |

When the parser is disabled, call `osrx_sensor_recv(buf, len, &meta, &field)` directly on the complete datagram buffer.

---

### `OSRX_NO_TIMESTAMP`

Default: `0`

When `1`:
- `osrx_packet_meta.ts_sec` field is removed from the struct.
- The 6-byte timestamp field in the wire frame is still read (for body offset calculation) but discarded.
- Flash savings: **~20 B** on AVR; **RAM savings: 4 B** per `osrx_packet_meta` instance.

> This is the lowest-risk switch: timestamps are rarely needed in leaf-node receivers that process data immediately. They are primarily useful for centralized gateways that log or time-order frames.

---

## 4. Combining Switches — Trade-off Matrix

| Goal | Recommended combination | Flash (AVR) | RAM saved vs default |
|------|------------------------|-------------|---------------------|
| Smallest possible binary | All four switches | ~310 B | 4 B (ts_sec) + 102 B (parser) |
| UART streaming, no ts | OSRX_NO_TIMESTAMP | ~605 B | 4 B |
| UDP datagram, full validation | OSRX_NO_PARSER | ~442 B | 102 B |
| UDP, trusted link, tiny RAM | OSRX_NO_PARSER + both CRC off | ~321 B | 102 B |
| UDP, no ts, no CRC | All four switches | ~310 B | 106 B |

---

## 5. Verifying What Was Compiled

### CMake / GCC toolchain

After building:

```bash
avr-nm -C --size-sort build/libosrx.a | sort -k1 -r | head -20
```

Locate `osrx_crc8`, `osrx_crc16`, `osrx_feed_byte`, `ts_sec` in the symbol output. A missing symbol confirms the switch worked.

### On Linux/macOS (x86-64, for pre-flight sizing)

```bash
cmake -B build -DOSRX_NO_PARSER=1 -DOSRX_NO_TIMESTAMP=1 \
      -DOSRX_VALIDATE_CRC8=0 -DOSRX_VALIDATE_CRC16=0 \
      -DCMAKE_BUILD_TYPE=MinSizeRel
cmake --build build
size build/libosrx.a
```

Then apply the 0.55 estimate factor for AVR.

---

## 6. Why No Lookup Tables

A common CRC-16 implementation uses a 256-entry × 2-byte lookup table (512 B RAM). On AVR:

| Approach | Flash cost | RAM cost | AVR RAM % (328P) |
|----------|-----------|---------|-----------------|
| Bit-loop (OSynaptic-RX) | ~72 B | 0 B | 0% |
| Lookup table | ~16 B (ptr + loop) | 512 B | **25%** |

OSynaptic-RX uses the bit-loop because 25% of the ATmega328P's SRAM is an unacceptable cost for CRC computation on embedded sensor nodes. The extra ~56 B Flash cost is negligible.

If throughput is a concern on a 32-bit target with abundant RAM (ESP32, STM32F103), you may disable `OSRX_VALIDATE_CRC16` and substitute your own hardware-accelerated or table-based CRC-16 check.

---

## 7. `#if !OSRX_NO_TIMESTAMP` Pattern

The timestamp guard follows the same compile-time exclusion pattern used across the codebase. Here is a minimal illustration:

```
┌─────────────────────────────┐
│  osrx_config.h              │
│  #ifndef OSRX_NO_TIMESTAMP  │
│  #define OSRX_NO_TIMESTAMP 0│
│  #endif                     │
└──────────────┬──────────────┘
               │
               ▼
┌─────────────────────────────┐
│  osrx_packet.h              │
│  typedef struct {           │
│    ...                      │
│  #if !OSRX_NO_TIMESTAMP     │
│    osrx_u32 ts_sec;         │
│  #endif                     │
│  } osrx_packet_meta;        │
└──────────────┬──────────────┘
               │
               ▼
┌─────────────────────────────┐
│  osrx_packet.c              │
│  #if !OSRX_NO_TIMESTAMP     │
│    meta->ts_sec = ...;      │
│  #endif                     │
└─────────────────────────────┘
```

**Properties**:

| Property | Value |
|----------|-------|
| C standard | C89 (no `_Static_assert`, no variadic macros) |
| Compile-time only | Yes — zero runtime overhead |
| Flash cost | 0 B |
| RAM cost | 0 B |
| Toolchain support | All (GCC, Clang, IAR, MSVC, AVR-GCC) |
| ABI note | `osrx_packet_meta` layout differs between `OSRX_NO_TIMESTAMP=0` and `=1`; all translation units in a binary must be compiled with the same setting |

> **ABI Warning**: if your application code and OSynaptic-RX are compiled with different values of `OSRX_NO_TIMESTAMP`, the struct layout will disagree and cause silent data corruption. Always set macros through your build system so they apply to all files at once.
