/*
 * LoRaRX.ino -- OSynaptic-RX over LoRa (SX1276/SX1278 via LoRa.h)
 * =================================================================
 * Each LoRa packet holds exactly one complete OpenSynaptic wire frame,
 * so no streaming parser is needed. The receive callback passes the
 * entire buffer directly to osrx_sensor_recv().
 *
 * This example compiles with OSRX_NO_PARSER=1 (no OSRXParser struct).
 * RAM saving: 102 B compared with the streaming-parser examples.
 *
 * Library required: "LoRa" by Sandeep Mistry
 *   Arduino IDE -> Sketch > Include Library > Manage Libraries -> search "LoRa"
 *
 * Compatible boards:
 *   - Heltec LoRa 32 (ESP32 + SX1276)
 *   - TTGO LoRa32 (ESP32 + SX1276)
 *   - Arduino Uno + LoRa shield (SX1276)
 *   - Any AVR/ESP32 board with SX1276/SX1278 on SPI
 *
 * Wiring (typical Heltec LoRa 32 v2):
 *   SCK  = GPIO5   MISO = GPIO19   MOSI = GPIO27
 *   SS   = GPIO18  RST  = GPIO14   DIO0 = GPIO26
 *
 * Memory (ATmega328P, OSRX_NO_PARSER=1, -Os):
 *   Static RAM : receive buffer udp_buf (OSRX_PACKET_MAX = 96 B)
 *                osrx_packet_meta (~19 B) + osrx_sensor_field (~22 B) on stack
 *   Flash total: ~442 B (no parser, CRC validation on)
 *
 * LoRa notes:
 *   - SF7  / BW125 / CR4/5 : a 30-byte payload takes ~15 ms air time
 *   - SF12 / BW125 / CR4/8 : same payload takes ~2.7 s
 *   - Plan send intervals to stay within regional duty-cycle limits.
 *
 * Matching sender: OSynaptic-TX LoRaTX.ino (same frequency + settings).
 */

/* --- OSRX_NO_PARSER: exclude streaming parser, save 102 B RAM --- */
#ifndef OSRX_NO_PARSER
#define OSRX_NO_PARSER 1
#endif

#include <SPI.h>
#include <LoRa.h>
#include <OSynaptic-RX.h>

/* ------------------------------------------------------------------
 * LoRa pin mapping -- uncomment / adjust for your board
 * ------------------------------------------------------------------ */
/* Heltec LoRa 32 v2 */
#define LORA_SCK  5
#define LORA_MISO 19
#define LORA_MOSI 27
#define LORA_SS   18
#define LORA_RST  14
#define LORA_DIO0 26

/* Arduino Uno + LoRa shield (uncomment and comment the Heltec block):
#define LORA_SCK  13
#define LORA_MISO 12
#define LORA_MOSI 11
#define LORA_SS   10
#define LORA_RST  9
#define LORA_DIO0 2
*/

/* LoRa frequency -- match your region's ISM band */
static const long LORA_FREQ = 915E6;  /* 915 MHz (Americas). Use 868E6 for EU. */

/* ------------------------------------------------------------------
 * setup()
 * ------------------------------------------------------------------ */
void setup(void)
{
    Serial.begin(115200);
    while (!Serial) { ; }

    SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
    LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);

    if (!LoRa.begin(LORA_FREQ)) {
        Serial.println(F("LoRa init failed! Check wiring."));
        while (true) { ; }
    }

    Serial.println(F("OSynaptic-RX LoRa receiver ready"));
}

/* ------------------------------------------------------------------
 * loop() -- check for an incoming LoRa packet on each iteration
 * ------------------------------------------------------------------ */
static osrx_u8 s_rx_buf[OSRX_PACKET_MAX];

void loop(void)
{
    int pkt_size = LoRa.parsePacket();
    if (pkt_size <= 0) return;

    /* Read the complete LoRa packet into a local buffer.
       Discard if it overflows our maximum frame size. */
    int len = 0;
    while (LoRa.available() && len < OSRX_PACKET_MAX)
        s_rx_buf[len++] = (osrx_u8)LoRa.read();

    /* Flush any remaining bytes (oversize packet -- should not happen) */
    while (LoRa.available()) { LoRa.read(); }

    if (len <= 0) return;

    /*
     * osrx_sensor_recv() decodes the complete frame:
     *   1. Validates header structure
     *   2. Verifies CRC-8 (body) and CRC-16 (full frame)
     *   3. Parses "sensor_id|unit|b62value" body
     * Returns 1 only when ALL three steps succeed.
     */
    osrx_packet_meta  meta;
    osrx_sensor_field field;

    if (!osrx_sensor_recv(s_rx_buf, len, &meta, &field)) {
        /* Structural error or CRC mismatch */
        Serial.print(F("[LoRaRX] bad frame len="));
        Serial.println(len);
        return;
    }

    /* Received signal quality (RSSI / SNR from LoRa.h) */
    int rssi = LoRa.packetRssi();
    int snr  = (int)LoRa.packetSnr();   /* rounded to int for Serial.print */

    /* Integer-only value: scaled / OSRX_VALUE_SCALE */
    osrx_i32 whole = field.scaled / (osrx_i32)OSRX_VALUE_SCALE;
    osrx_i32 frac  = field.scaled % (osrx_i32)OSRX_VALUE_SCALE;
    if (frac < 0L) { frac = -frac; }

    Serial.print(F("[LoRaRX] aid="));
    Serial.print((unsigned long)meta.aid, HEX);
    Serial.print(F(" rssi="));  Serial.print(rssi);
    Serial.print(F(" snr="));   Serial.print(snr);
    Serial.print(F("  "));
    Serial.print(field.sensor_id);
    Serial.print(F(" = "));
    Serial.print((long)whole);
    Serial.print('.');
    if (frac < 1000L) Serial.print('0');
    if (frac < 100L)  Serial.print('0');
    if (frac < 10L)   Serial.print('0');
    Serial.print((long)frac);
    Serial.print(' ');
    Serial.println(field.unit);
}
