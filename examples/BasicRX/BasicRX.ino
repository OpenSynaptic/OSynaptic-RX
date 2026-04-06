/*
 * BasicRX.ino -- OSynaptic-RX streaming parser example (Arduino AVR/Uno)
 * ========================================================================
 * Hardware: Arduino Uno / Nano (ATmega328P @ 16 MHz)
 * Protocol: OpenSynaptic wire format v1 (FULL frame, 16+ bytes)
 * Scenario: Receive sensor frames from an OSynaptic-TX node via hardware
 *           Serial (or SoftwareSerial on other pins).
 *
 * Wire format reminder:
 *   [cmd:1][route:1][aid:4BE][tid:1][ts:6BE][body][crc8:1][crc16:2BE]
 *   body = "<sensor_id>|<unit>|<b62(value)>"
 *   total = 13 + body_len + 3 bytes
 *
 * Frame boundary detection:
 *   This example uses a UART idle-gap timeout: if no byte arrives for
 *   FRAME_TIMEOUT_MS milliseconds the accumulated bytes are treated as
 *   one complete frame and passed to osrx_feed_done().
 *
 * Integer-only output (no float):
 *   real_value = field.scaled / OSRX_VALUE_SCALE
 *               e.g. 215000 / 10000 = "21.5000"
 */

#include <OSynaptic-RX.h>

/* ---------- tunables ---------------------------------------------- */
#define BAUD_RATE        9600UL
#define FRAME_TIMEOUT_MS 15        /* ms of silence before frame_done  */
/* ------------------------------------------------------------------ */

static OSRXParser g_parser;
static unsigned long g_last_byte_ms = 0;
static int        g_got_byte        = 0;

/* ------------------------------------------------------------------ */
/* Frame callback -- called once per complete parsed frame            */
/* ------------------------------------------------------------------ */
static void on_frame(
    const osrx_packet_meta  *meta,
    const osrx_sensor_field *field,
    const osrx_u8           *raw,
    int                      raw_len,
    void                    *ctx)
{
    (void)raw;
    (void)raw_len;
    (void)ctx;

    if (!meta->crc8_ok || !meta->crc16_ok) {
        Serial.println(F("ERR: CRC mismatch"));
        return;
    }

    if (!field) {
        Serial.println(F("ERR: body parse failed"));
        return;
    }

    /* Integer-only value display */
    osrx_i32 whole = field->scaled / (osrx_i32)OSRX_VALUE_SCALE;
    osrx_i32 frac  = field->scaled % (osrx_i32)OSRX_VALUE_SCALE;
    if (frac < 0L) { frac = -frac; }

    Serial.print(F("aid="));   Serial.print((unsigned long)meta->aid, HEX);
    Serial.print(F(" ts="));   Serial.print((unsigned long)meta->ts_sec);
    Serial.print(F(" id="));   Serial.print(field->sensor_id);
    Serial.print(F(" val="));  Serial.print((long)whole);
    Serial.print('.');
    /* Print exactly 4 decimal places. */
    if (frac < 1000L) Serial.print('0');
    if (frac < 100L)  Serial.print('0');
    if (frac < 10L)   Serial.print('0');
    Serial.print((long)frac);
    Serial.print(' ');
    Serial.println(field->unit);
}

/* ------------------------------------------------------------------ */
void setup(void)
{
    Serial.begin(BAUD_RATE);
    osrx_parser_init(&g_parser, on_frame, 0);
    Serial.println(F("OSynaptic-RX ready"));
}

void loop(void)
{
    /* Feed incoming bytes to the streaming parser */
    while (Serial.available() > 0) {
        osrx_u8 b = (osrx_u8)Serial.read();
        osrx_feed_byte(&g_parser, b);
        g_last_byte_ms = millis();
        g_got_byte     = 1;
    }

    /* Idle gap = end of frame */
    if (g_got_byte && (millis() - g_last_byte_ms) >= FRAME_TIMEOUT_MS) {
        osrx_feed_done(&g_parser);
        g_got_byte = 0;
    }
}
