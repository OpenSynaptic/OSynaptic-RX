# 01 — Deployment Guide

This document covers hardware selection, memory sizing, transport selection, and buffer-tuning considerations for deploying OSynaptic-RX on 8-bit MCUs and embedded Linux targets.

---

## 1. Minimum Hardware Requirements

| Resource | Absolute Minimum | Recommended Minimum |
|----------|-----------------|-------------------|
| Flash | 0.7 KB (`OSRX_NO_PARSER`, `MinSizeRel`) | 2 KB (full library) |
| RAM | 16 B (no parser, `OSRXParser` not instantiated) | 256 B (parser + frame buffer) |
| Inputs | Any transport that delivers a byte stream or fixed-length datagrams | Hardware UART / SPI / I²C |
| Clock | Any (no timing dependency in library) | 8–16 MHz for reliable UART baud rates |

---

## 2. AVR Family Deployment Table

### 8-bit AVR — receiver (gateway / display / hub) deployments

| MCU | Flash | RAM | UART type | Parser mode | Config tier | Notes |
|-----|-------|-----|-----------|-------------|-------------|-------|
| ATtiny85 | 8 KB | 512 B | USI/SW only | No-parser (UDP) | **Tight** | Parser disabled; feed full datagram via `osrx_feed_bytes`. |
| ATmega48 | 4 KB | 512 B | HW UART0 | No-parser | **Tight** | Suitable for simple display or relay node. |
| ATmega88 | 8 KB | 1 KB | HW UART0 | Full parser | **Standard** | Comfortable with parser and 96 B frame buffer. |
| ATmega168 | 16 KB | 1 KB | HW UART0 | Full parser | **Standard** | Spare Flash for application logic. |
| **ATmega328P** | **32 KB** | **2 KB** | **HW UART0** | **Full parser** | **Standard** | Reference platform; recommended for all new gateway designs. |
| ATmega32U4 | 32 KB | 2.5 KB | HW UART + USB | Full parser | **Standard** | USB-CDC bridging possible (USB CDC → `osrx_feed_byte`). |
| ATmega2560 | 256 KB | 8 KB | 4× HW UART | Full parser | **Comfort** | Multi-channel receiver (1 UART per sensor cluster). |
| ATmega4809 | 48 KB | 6 KB | 4× USART | Full parser | **Comfort** | tinyAVR-2 core; UPDI programming. |

> **Note on parser mode**: "No-parser" means `OSRX_NO_PARSER=1`; the `osrx_sensor_recv()` / `osrx_sensor_unpack()` / `osrx_packet_decode()` APIs remain fully available. "No-parser" saves ~316 B Flash and is correct whenever transport framing provides natural frame boundaries (UDP, SPI with CS, I²C with stop-bit).

> **Note on `OSRXParser` RAM**: the struct occupies `96 (frame buffer) + 2 (len) + 4 (pointers)` = **102 B** on AVR. This is the dominant RAM cost on tight targets.

### Flash usage by build profile (AVR estimate, `-Os`)

| Profile | Flags | x86-64 text | AVR estimate |
|---------|-------|-------------|--------------|
| Full | *(defaults)* | 1120 B | ~616 B |
| No parser | `OSRX_NO_PARSER=1` | ~804 B | ~442 B |
| No CRC + No parser | `OSRX_NO_PARSER=1 OSRX_VALIDATE_CRC8=0 OSRX_VALIDATE_CRC16=0` | ~584 B | ~321 B |

> AVR estimates use the 0.55× empirical ratio (AVR 8-bit vs x86-64 `-Os`). See [05-flash-optimization.md](05-flash-optimization.md) for detailed breakdown.

---

## 3. 32-bit Targets

| MCU | Flash | RAM | Parser mode | Notes |
|-----|-------|-----|-------------|-------|
| STM32F030F4 | 16 KB | 4 KB | Full parser | Cortex-M0; all APIs fit comfortably. |
| STM32F103C8 | 64 KB | 20 KB | Full parser | "Blue Pill"; Ethernet via ENC28J60 for LAN gateway. |
| ESP8266 | 1–4 MB | 80 KB | Full parser | UDP receive via `WiFiUDP`; one datagram per `osrx_feed_bytes` call. |
| **ESP32** | **4 MB** | **520 KB** | **Full parser** | **Preferred LAN gateway platform.** |
| RP2040 | 2 MB | 264 KB | Full parser | PIO can implement custom UART framing; feed bytes via DMA callback. |
| Linux / macOS | — | — | Full parser | `cmake -B build && cmake --build build`; run native. |

---

## 4. Transport Selection

### 4.1 UART / RS-485 (wired, streaming — use parser)

Use when:
- MCU has hardware UART and is within cable reach of the TX node
- Frame boundaries are indicated by idle-gap (≥ 10 ms gap between frames)

```c
/* Feed bytes in ISR or polling loop */
void uart_rx_isr(void) {
    osrx_feed_byte(&parser, UDR0);
}

/* Idle gap detection in main loop */
if (millis() - last_byte_ms > 15) {
    osrx_feed_done(&parser);
    last_byte_ms = millis();
}
```

For distances > 10 m: add a MAX485 / SN75176 RS-485 transceiver. Library output is identical; only the physical layer changes.

### 4.2 WiFi UDP (ESP32 / ESP8266, no parser needed)

Use when:
- MCU is ESP32 or ESP8266
- TX node sends one UDP datagram per frame

```cpp
int n = udp.parsePacket();
if (n > 0) {
    uint8_t buf[OSRX_PACKET_MAX];
    int len = udp.read(buf, sizeof(buf));
    osrx_feed_bytes(&parser, buf, len);  /* feed_bytes calls feed_done internally */
}
```

**Why UDP suits RX**: each datagram is a complete frame; no idle-gap detection required. `OSRX_NO_PARSER=1` is safe here because `osrx_sensor_recv()` accepts the complete datagram directly.

### 4.3 SPI / I²C bridge (frame-boundary via CS or stop-bit)

Use when:
- TX node is on a short PCB-to-PCB link
- SPI slave-select or I²C stop condition marks frame end

```c
void spi_cs_rising_isr(void) {
    osrx_feed_done(&parser);   /* CS rising = end of frame */
}

void spi_byte_received_isr(uint8_t byte) {
    osrx_feed_byte(&parser, byte);
}
```

### 4.4 LoRa (short frames, no parser needed)

Use when:
- Gateway node receives LoRa datagrams
- Frame is already complete in the LoRa receive buffer

```c
void lora_on_receive(int pkt_size) {
    uint8_t buf[OSRX_PACKET_MAX];
    int i;
    for (i = 0; i < pkt_size && i < OSRX_PACKET_MAX; ++i)
        buf[i] = (uint8_t)LoRa.read();
    osrx_sensor_recv(buf, pkt_size, &meta, &field);
}
```

---

## 5. `OSRXParser` Sizing

The `OSRXParser` struct holds one complete frame in a statically allocated buffer:

```
OSRXParser
├── buf[OSRX_PACKET_MAX]   96 B  frame accumulation buffer
├── len                     2 B  bytes accumulated so far (uint16 on AVR)
├── cb                      2 B  callback function pointer
└── ctx                     2 B  user context pointer
                          ════
Total (AVR int=2B, ptr=2B)  102 B
```

If you declare `OSRXParser` as a local variable, it consumes 102 B of stack in addition to whatever the callback uses.

**Recommended**: declare `OSRXParser` at file scope (global/static):

```c
static OSRXParser g_parser;   /* zero-init at startup; no stack cost */
```

### Tuning `OSRX_PACKET_MAX`

Reducing `OSRX_PACKET_MAX` saves RAM at the cost of maximum supported frame size:

| `OSRX_PACKET_MAX` | `OSRXParser` RAM | Max frame body |
|------------------|-----------------|----------------|
| 96 (default) | 102 B | 80 B |
| 64 | 70 B | 48 B |
| 48 | 54 B | 32 B |
| 32 | 38 B | 16 B |

Minimum: header (13 B) + minimal body (7 B: `"X|Y|0"`) + CRC (3 B) = **23 B**, so `OSRX_PACKET_MAX=32` is tight but functional for ultra-constrained sensors.

---

## 6. Callback Design

The frame-complete callback fires inside `osrx_feed_done()` from the call-site's context (ISR, main loop, or RTOS task). Keep it short:

```c
void on_frame(const osrx_packet_meta  *meta,
              const osrx_sensor_field *field,
              const osrx_u8 *raw, int raw_len, void *ctx)
{
    /* 1. Validate CRCs */
    if (!meta->crc8_ok || !meta->crc16_ok) return;
    /* 2. Validate command byte */
    if (meta->cmd != OSRX_CMD_DATA_FULL)   return;
    /* 3. Check field was decoded */
    if (!field) return;

    /* 4. Use the data */
    /* field->sensor_id, field->unit, field->scaled */
}
```

> **Do not call `osrx_feed_byte()` or `osrx_feed_done()` from inside the callback** — the parser is mid-dispatch and re-entrancy is not supported.
