# OSynaptic-RX

**面向 8 位 MCU 的 OpenSynaptic 单向接收解码器**。它用 **纯 C89**、**无堆内存**、AVR 上最低 **55 字节栈峰值** 的实现，把 UART / UDP / LoRa / RS-485 / SPI 等串行链路上的 OpenSynaptic 传感器帧解码为经过校验的定点整数读数。它可以直接对接 [OSynaptic-TX](../OSynaptic-TX/README.md) 节点和 [OpenSynaptic](../OpenSynaptic/README.md) Python Hub。

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

## 快速参考表

| 表格 | 说明 |
|---|---|
| [MCU 配置参考](docs/04-mcu-config-reference.md) | AVR、STM32、ESP 等 MCU 家族的推荐配置档、Flash/RAM 预算、`OSRX_PACKET_MAX` 建议 |
| [Flash 优化指南](docs/05-flash-optimization.md) | 8 种配置组合的 Flash/RAM 实测值、按开关的裁剪建议、CRC 取舍说明 |

如果你刚接触这个库，先看下面的快速开始；如果你要在特定 MCU 上部署，先看这两张表最省时间。

---

## 30 秒上手

```text
Arduino IDE → Sketch > Include Library > Add .ZIP Library → 选择 OSynaptic-RX.zip
File > Examples > OSynaptic-RX > BasicRX → Upload
```

把 OSynaptic-TX 节点接到 RX 板的串口上，以 **9600 波特率** 打开串口监视器，即可看到每帧对应的解码读数。

---

## 目录

- [为什么选择 OSynaptic-RX](#为什么选择-osynaptic-rx)
- [两种解码路径](#两种解码路径)
- [内存占用](#内存占用)
- [MCU 部署参考](#mcu-部署参考)
- [传输选择指南](#传输选择指南)
- [快速开始](#快速开始)
- [线协议格式](#线协议格式)
- [API 参考](#api-参考)
- [配置](#配置)
- [示例](#示例)
- [仓库结构](#仓库结构)
- [CMake 构建](#cmake-构建)
- [测试结果](#测试结果)
- [文档](#文档)
- [贡献](#贡献)
- [许可证](#许可证)

---

## 为什么选择 OSynaptic-RX

- **只收不发**：不带编码逻辑，Flash 成本远低于全双工实现。
- **C89 干净**：适合 avr-gcc、SDCC、IAR、MPLAB XC8 等面向 8 位 MCU 的工具链。
- **无堆内存**：没有 `malloc` / `free`，状态保存在栈或全局 `OSRXParser` 中。
- **无浮点依赖**：值以定点整数形式解码，真实值通过 `field->scaled / OSRX_VALUE_SCALE` 还原。
- **双 CRC 校验**：body 用 CRC-8/SMBUS，整帧用 CRC-16/CCITT-FALSE。
- **协议兼容**：能直接解码 OSynaptic-TX 与 OpenSynaptic Hub 生成的帧。

---

## 两种解码路径

| 路径 | API | 适用场景 |
|---|---|---|
| 流式解析器 | `osrx_feed_byte()` + `osrx_feed_done()` | UART、USB CDC、带空闲间隔的串口流 |
| 直接整帧解码 | `osrx_sensor_recv()` | UDP、LoRa、SPI 这类天然有帧边界的传输 |

流式解析器使用 `OSRXParser`，在 AVR 上约占 102 B RAM。对于天然一包一帧的传输，建议开启 `OSRX_NO_PARSER=1`，这样可以省下 102 B RAM 和约 316 B Flash。

---

## 内存占用

### 流式解析器（UART / RS-485）

| 资源 | 占用 |
|---|---|
| `OSRXParser` 静态 RAM | 102 B |
| 回调链栈峰值 | 约 55 B |
| Flash（全默认、`-Os`） | 约 616 B |

### 直接整帧解码（UDP / LoRa，`OSRX_NO_PARSER=1`）

| 资源 | 占用 |
|---|---|
| `OSRXParser` RAM | **0 B** |
| `osrx_sensor_recv` 栈峰值 | 约 41 B |
| Flash（`-Os`） | 约 442 B |

### 最低支持 MCU

| 模式 | 最低 RAM | 最低 Flash | 示例 |
|---|---|---|---|
| 流式解析 | ≥ 256 B | ≥ 2 KB | ATmega88 / ATmega168 |
| 无解析器 | ≥ 64 B | ≥ 1 KB | ATtiny85 / ATmega48 |

---

## MCU 部署参考

### 8 位 AVR 接收节点

| MCU | Flash | RAM | UART | 解析模式 | 配置档 | 说明 |
|---|---|---|---|---|---|---|
| ATtiny85 | 8 KB | 512 B | USI/SW | **无解析器** | Tight | 对整帧 LoRa/UDP 很合适 |
| ATmega48 | 4 KB | 512 B | HW UART0 | 无解析器 | Tight | 512 B RAM 场景建议关闭解析器 |
| ATmega88 | 8 KB | 1 KB | HW UART0 | 完整解析器 | Standard | 首个较舒适的平台 |
| **ATmega328P** | **32 KB** | **2 KB** | **HW UART0** | **完整解析器** | **Standard** | **Uno / Nano 推荐基线** |
| ATmega2560 | 256 KB | 8 KB | 4× HW UART | 完整解析器 | Comfort | 多通道 Hub 场景理想 |

### 32 位平台

| MCU | Flash / RAM | 解析模式 | 说明 |
|---|---|---|---|
| STM32F030F4 | 16 KB / 4 KB | 完整解析器 | 小型 Cortex-M0 |
| STM32F103C8 | 64 KB / 20 KB | 完整解析器 | 适合多串口聚合 |
| ESP8266 | 1–4 MB / 80 KB | 完整解析器 | 可直接一包一帧喂入 |
| **ESP32** | **4 MB / 520 KB** | **完整解析器** | **推荐局域网接收网关** |
| RP2040 | 2 MB / 264 KB | 完整解析器 | 适合 DMA / PIO 配合 |

---

## 传输选择指南

### UART / RS-485（使用流式解析器）

适合 AVR / STM32 到 TX 设备有线直连的场景。使用 `osrx_feed_byte()` 持续喂字节，在 15 ms 空闲间隔后调用 `osrx_feed_done()` 触发一帧解析。

### WiFi UDP（ESP32 / ESP8266，无需解析器）

每个 UDP datagram 就是一帧。直接把 `parsePacket()` 读到的缓冲区交给 `osrx_feed_bytes()` 或 `osrx_sensor_recv()` 即可。

### LoRa（SX1276 / SX1278，无需解析器）

每个 LoRa 包即一帧，可在 `onReceive` 中直接调用 `osrx_sensor_recv()`。这类链路通常天然有帧边界。

### 多通道接收（ATmega2560 / STM32）

适合做 Hub。每个串口绑定一个独立 `OSRXParser`：

```c
static OSRXParser chan[4];
void uart0_isr(void) { osrx_feed_byte(&chan[0], UDR0); }
```

---

## 快速开始

### Arduino：UART 流式接收

```cpp
#include <OSynaptic-RX.h>

static OSRXParser parser;

static void on_frame(const osrx_packet_meta  *meta,
                     const osrx_sensor_field *field,
                     const osrx_u8 *, int, void *)
{
    if (!meta->crc8_ok || !meta->crc16_ok) return;
    if (!field) return;

    Serial.print(field->sensor_id);
    Serial.print(": ");
    Serial.print((long)(field->scaled / OSRX_VALUE_SCALE));
    Serial.print(" ");
    Serial.println(field->unit);
}

void setup() {
    Serial.begin(9600);
    osrx_parser_init(&parser, on_frame, nullptr);
}
```

### ESP32 UDP：一包一帧

```cpp
#include <OSynaptic-RX.h>

static uint8_t udp_buf[OSRX_PACKET_MAX];

void loop() {
    int n = udp.parsePacket();
    if (n > 0) {
        int len = udp.read((char*)udp_buf, n < OSRX_PACKET_MAX ? n : OSRX_PACKET_MAX);
        osrx_packet_meta meta;
        osrx_sensor_field field;
        if (osrx_sensor_recv(udp_buf, len, &meta, &field)) {
            /* field.sensor_id / field.unit / field.scaled */
        }
    }
}
```

### 原生 C

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

## 线协议格式

所有帧都遵循 OpenSynaptic FULL 包格式：

```text
[cmd:1][route:1][aid:4BE][tid:1][ts:6BE][sid|unit|b62][crc8:1][crc16:2]
```

| 字段 | 大小 | 说明 |
|---|---|---|
| `cmd` | 1 B | `0x3F` 为明文 FULL 数据；`0x40` 为加密帧，RX 默认不解 |
| `route` | 1 B | 路由 / hop 标志 |
| `aid` | 4 B | 源 agent ID，大端 |
| `tid` | 1 B | 事务 ID |
| `ts` | 6 B | Unix 时间戳秒，48 位大端 |
| body | 可变 | `sensor_id|unit|b62value` |
| `crc8` | 1 B | body 的 CRC-8/SMBUS |
| `crc16` | 2 B | 全帧 CRC-16/CCITT-FALSE |

最小帧长约为 **19 字节**。

---

## API 参考

### `osrx_sensor.h`

```c
int osrx_sensor_recv(const osrx_u8 *packet, int len,
                     osrx_packet_meta *meta, osrx_sensor_field *field);

int osrx_sensor_unpack(const osrx_u8 *body, int body_len,
                       osrx_sensor_field *out);
```

### `osrx_parser.h`

| 函数 | 说明 |
|---|---|
| `osrx_parser_init(p, cb, ctx)` | 初始化解析器并注册回调 |
| `osrx_feed_byte(p, b)` | 喂入一个字节 |
| `osrx_feed_done(p)` | 通知帧结束并触发解析 |
| `osrx_feed_bytes(p, data, len)` | 批量喂入并自动结束 |
| `osrx_parser_reset(p)` | 丢弃当前缓存 |

### `osrx_packet.h`

```c
int osrx_packet_decode(const osrx_u8 *packet, int len, osrx_packet_meta *out);
```

### `osrx_b62.h`

```c
osrx_i32 osrx_b62_decode(const char *s, int len, int *ok);
```

---

## 配置

配置位于 [src/osrx_config.h](src/osrx_config.h)，可通过 CMake `-D` 或包含前 `#define` 覆盖。

| 宏 | 默认值 | 含义 |
|---|---|---|
| `OSRX_PACKET_MAX` | 96 | 最大帧长 |
| `OSRX_ID_MAX` | 9 | 传感器 ID 最大长度 |
| `OSRX_UNIT_MAX` | 9 | 单位最大长度 |
| `OSRX_B62_MAX` | 14 | Base62 字符串最大长度 |
| `OSRX_BODY_MAX` | 64 | body 最大字节数 |
| `OSRX_VALUE_SCALE` | 10000 | 定点缩放系数 |
| `OSRX_VALIDATE_CRC8` | 1 | 是否校验 body CRC |
| `OSRX_VALIDATE_CRC16` | 1 | 是否校验整帧 CRC |
| `OSRX_NO_PARSER` | 0 | 是否裁掉流式解析器 |
| `OSRX_NO_TIMESTAMP` | 0 | 是否移除时间戳字段 |

---

## 示例

| 示例 | 传输 | 解析方式 | 目标 |
|---|---|---|---|
| [BasicRX](examples/BasicRX/BasicRX.ino) | UART | 流式 | 任意 Arduino |
| [MultiSensorRX](examples/MultiSensorRX/MultiSensorRX.ino) | UART | 流式 | 多传感器派发 |
| [ESP32UdpRX](examples/ESP32UdpRX/ESP32UdpRX.ino) | WiFi UDP | `osrx_feed_bytes` | ESP32 / ESP8266 |
| [LoRaRX](examples/LoRaRX/LoRaRX.ino) | LoRa | `osrx_sensor_recv` | Heltec / TTGO / Uno + shield |
| [BareMetalUARTRX](examples/BareMetalUARTRX/BareMetalUARTRX.ino) | USART0 寄存器 | 流式 | ATmega328P |

---

## 仓库结构

```text
OSynaptic-RX/
├── OSynaptic-RX.h
├── library.properties
├── docs/
│   ├── 01-deployment-guide.md
│   ├── 02-api-reference.md
│   ├── 03-wire-format.md
│   ├── 04-mcu-config-reference.md
│   └── 05-flash-optimization.md
├── src/
│   ├── osrx_config.h
│   ├── osrx_types.h
│   ├── osrx_crc.h/c
│   ├── osrx_b62.h/c
│   ├── osrx_packet.h/c
│   ├── osrx_sensor.h/c
│   ├── osrx_parser.h/c
│   └── OSynaptic-RX.h
├── include/
├── examples/
├── tests/
└── CMakeLists.txt
```

---

## CMake 构建

需要 CMake ≥ 3.10 和支持 C89 的编译器。

```powershell
cmake -B build
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

安装：

```powershell
cmake --install build --prefix /usr/local
```

通过 `find_package(osrx 1.0 REQUIRED)` 即可在其他 CMake 项目中使用 `osrx::osrx`。

---

## 测试结果

运行：

```sh
cmake -B build -DOSRX_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

测试覆盖 CRC、Base62、帧头解码、sensor unpack、`OSRX_NO_TIMESTAMP` 结构布局等，共 **39 条断言**，预期结果为 **39 passed, 0 failed**。

---

## 文档

完整文档位于 [docs/](docs/)：

| 文件 | 内容 |
|---|---|
| [docs/01-deployment-guide.md](docs/01-deployment-guide.md) | 硬件目标、传输适配器、`OSRXParser` 规模 |
| [docs/02-api-reference.md](docs/02-api-reference.md) | 参数表、返回值与 AVR 栈占用 |
| [docs/03-wire-format.md](docs/03-wire-format.md) | 字节级线协议、CRC 规范、Base62 解码 |
| [docs/04-mcu-config-reference.md](docs/04-mcu-config-reference.md) | Ultra / Tight / Standard / Comfort 分档与 MCU 适配表 |
| [docs/05-flash-optimization.md](docs/05-flash-optimization.md) | Flash 优化矩阵与各开关效果 |

---

## 贡献

提交 PR 前请先阅读 [CONTRIBUTING.md](CONTRIBUTING.md) 与 [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md)。

---

## 许可证

Apache License 2.0。详见 [LICENSE](LICENSE)。