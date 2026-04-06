# 04 — MCU Config Reference

This document provides recommended compile-time configuration tiers, per-MCU fit guidance, and buffer-sizing formulas for deploying OSynaptic-RX on constrained hardware.

---

## 1. Config Tiers

Four tiers balance functionality against Flash / RAM cost. Each tier is a set of `#define` overrides applied before including any OSynaptic-RX header.

| Tier | Target Flash | Target RAM | Parser | Timestamps | CRC-8 | CRC-16 |
|------|-------------|-----------|--------|-----------|-------|--------|
| **Ultra** | < 0.5 KB | < 64 B | Disabled | Disabled | Disabled | Disabled |
| **Tight** | < 1 KB | < 128 B | Optional | Disabled | Enabled | Enabled |
| **Standard** | < 4 KB | < 256 B | Enabled | Enabled | Enabled | Enabled |
| **Comfort** | Unrestricted | Unrestricted | Enabled | Enabled | Enabled | Enabled |

### Ultra Tier

Disable all optional features. Only `osrx_sensor_recv()` (or `osrx_packet_decode()` + `osrx_sensor_unpack()`) remain available.

```c
/* project_config.h or CMakeLists.txt -D flags */
#define OSRX_VALIDATE_CRC8   0
#define OSRX_VALIDATE_CRC16  0
#define OSRX_NO_PARSER       1
#define OSRX_NO_TIMESTAMP    1
#define OSRX_PACKET_MAX     32   /* shrink buffer to sensor-only minimum */
```

**CMake flags**:

```cmake
target_compile_definitions(myapp PRIVATE
    OSRX_VALIDATE_CRC8=0
    OSRX_VALIDATE_CRC16=0
    OSRX_NO_PARSER=1
    OSRX_NO_TIMESTAMP=1
    OSRX_PACKET_MAX=32
)
```

**Expected resources** (AVR `-Os`):

| Resource | Ultra |
|----------|-------|
| Flash | ~290 B |
| `OSRXParser` RAM | N/A (parser disabled) |
| `osrx_packet_meta` | 15 B |
| `osrx_sensor_field` | 22 B |

---

### Tight Tier

CRC validation on; parser optional (enable for UART streaming, disable for UDP/LoRa); no timestamps.

```c
#define OSRX_NO_TIMESTAMP    1
#define OSRX_PACKET_MAX     64   /* adequate for most sensors */
/* Leave OSRX_VALIDATE_CRC8/CRC16 at default 1 */
/* Set OSRX_NO_PARSER=1 if transport provides frame boundaries */
```

**Expected resources** (AVR `-Os`, parser enabled):

| Resource | Tight |
|----------|-------|
| Flash | ~450 B |
| `OSRXParser` RAM | 70 B (with `OSRX_PACKET_MAX=64`) |
| `osrx_packet_meta` | 15 B |
| `osrx_sensor_field` | 22 B |

---

### Standard Tier

```c
/* All defaults — no overrides needed.
   Optional: shrink packet buffer slightly */
/* #define OSRX_PACKET_MAX 96 */   /* default */
```

**Expected resources** (AVR `-Os`):

| Resource | Standard |
|----------|---------|
| Flash | ~616 B |
| `OSRXParser` RAM | 102 B |
| `osrx_packet_meta` | 19 B |
| `osrx_sensor_field` | 22 B |
| **Total stack peak** | **~157 B** |

---

### Comfort Tier

For 32-bit targets with abundant resources. No changes needed; add `-DOSRX_VALIDATE_CRC8=1 -DOSRX_VALIDATE_CRC16=1` to make intent explicit in your build.

---

## 2. Buffer Sizing Formulas

### `OSRXParser` RAM

$$
\text{OSRXParser RAM} = \texttt{OSRX\_PACKET\_MAX} + 2 + 2 \cdot \text{sizeof(ptr)}
$$

| Platform | sizeof(ptr) | RAM (default 96) |
|----------|-------------|------------------|
| AVR 8-bit | 2 B | **102 B** |
| ARM Cortex-M0 | 4 B | **108 B** |
| ARM Cortex-M4 / x86-64 | 4 / 8 B | 108 / 120 B |

### Minimum `OSRX_PACKET_MAX`

$$
\texttt{OSRX\_PACKET\_MAX} \geq 13 + \texttt{OSRX\_BODY\_MAX} + 3
$$

Where 13 = header, 3 = CRC bytes. With defaults (`OSRX_BODY_MAX = 64`): min = 80.

Default `OSRX_PACKET_MAX = 96` provides 16 B of headroom.

### Minimum `OSRX_BODY_MAX`

$$
\texttt{OSRX\_BODY\_MAX} \geq \texttt{sensor\_id\_len} + 1 + \texttt{unit\_len} + 1 + \texttt{b62\_len}
$$

Where `b62_len` ≤ 13 for 32-bit signed integers (longest value: `-2147483648` in Base62 = 7 chars + `'-'` ).

---

## 3. Per-MCU Recommendations

### 8-bit AVR

| MCU | Flash | RAM | Recommended tier | `OSRX_PACKET_MAX` | Notes |
|-----|-------|-----|-----------------|-------------------|-------|
| ATtiny85 | 8 KB | 512 B | **Tight** (no parser) | 48 | USI/bit-bang; disable parser. |
| ATmega48 | 4 KB | 512 B | **Tight** | 64 | HW UART; only 512 B RAM total. |
| ATmega88 | 8 KB | 1 KB | **Standard** | 96 | Default config works. |
| **ATmega328P** | **32 KB** | **2 KB** | **Standard** | **96** | Reference platform. |
| ATmega32U4 | 32 KB | 2.5 KB | **Standard** | 96 | USB-CDC → feed_byte. |
| ATmega2560 | 256 KB | 8 KB | **Comfort** | 96 | Multiple parsers (one per UART). |
| ATmega4809 | 48 KB | 6 KB | **Comfort** | 96 | tinyAVR-2; UPDI. |

### Arm Cortex-M

| MCU | Flash | RAM | Recommended tier | Notes |
|-----|-------|-----|-----------------|-------|
| STM32F030F4 | 16 KB | 4 KB | **Standard** | — |
| STM32F103C8 | 64 KB | 20 KB | **Comfort** | Multiple parsers possible. |
| STM32L010F4 | 16 KB | 2 KB | **Tight** | Ultra-low-power; disable timestamp. |
| nRF52840 | 1 MB | 256 KB | **Comfort** | BLE → UART bridge common use-case. |
| RP2040 | 2 MB (ext.) | 264 KB | **Comfort** | PIO-based UART; DMA callback. |

### WiFi / BLE MCU

| MCU | Flash | RAM | Recommended tier | Notes |
|-----|-------|-----|-----------------|-------|
| ESP8266 | 4 MB | 80 KB | **Standard** | One UDP datagram = one `osrx_feed_bytes` call. |
| **ESP32** | **4 MB** | **520 KB** | **Comfort** | **Preferred gateway platform.** DualCore: one core runs UDP receive, one runs app logic. |
| W600 / W800 | 1 MB | 288 KB | **Comfort** | Community boards; UART bridging. |

---

## 4. Multi-Parser Pattern (multi-channel receiver)

When receiving from multiple independent UART channels, instantiate one `OSRXParser` per channel:

```c
static OSRXParser chan_parser[4];   /* 4 × 102 B = 408 B RAM on AVR */

void init_all(void)
{
    osrx_parser_init(&chan_parser[0], on_frame, (void *)0);
    osrx_parser_init(&chan_parser[1], on_frame, (void *)1);
    osrx_parser_init(&chan_parser[2], on_frame, (void *)2);
    osrx_parser_init(&chan_parser[3], on_frame, (void *)3);
}

/* In UART0 ISR */
void uart0_isr(void) { osrx_feed_byte(&chan_parser[0], UDR0); }

/* Frame callback uses ctx to identify the channel */
void on_frame(const osrx_packet_meta  *m,
              const osrx_sensor_field *f,
              const osrx_u8 *raw, int len, void *ctx)
{
    int channel = (int)(intptr_t)ctx;
    /* handle according to channel */
}
```

**RAM cost (ATmega2560)**: 4 parsers × 102 B = **408 B** out of 8192 B (5%).

---

## 5. CMake Configuration Quick-Reference

```cmake
# In CMakeLists.txt (or pass with -D on command line)

# -- Ultra Tier (ATtiny, absolute minimum) --
target_compile_definitions(${TARGET} PRIVATE
    OSRX_VALIDATE_CRC8=0 OSRX_VALIDATE_CRC16=0
    OSRX_NO_PARSER=1 OSRX_NO_TIMESTAMP=1
    OSRX_PACKET_MAX=32)

# -- Tight Tier (ATmega48/88, streaming UART) --
target_compile_definitions(${TARGET} PRIVATE
    OSRX_NO_TIMESTAMP=1 OSRX_PACKET_MAX=64)

# -- Standard Tier (ATmega328P, all defaults already) --
# No overrides needed. MinSizeRel is the default build type.

# -- Comfort Tier (ESP32, STM32F103, RP2040) --
# No overrides needed.
```

---

## 6. Arduino IDE Configuration

Arduino IDE does not use CMake. Override macros using a per-sketch header or the `build.extra_flags` in `boards.txt`.

Recommended approach — place a `osrx_user_config.h` file in your sketch folder:

```c
/* osrx_user_config.h */
#ifndef OSRX_NO_TIMESTAMP
  #define OSRX_NO_TIMESTAMP 1
#endif
#ifndef OSRX_PACKET_MAX
  #define OSRX_PACKET_MAX 64
#endif
```

Then include it before the library header:

```cpp
#include "osrx_user_config.h"
#include <osrx_parser.h>
#include <osrx_sensor.h>
```

> Arduino IDE compiles files in definition order; including your override before the library headers ensures the `#ifndef` guards in `osrx_config.h` pick up your values.
