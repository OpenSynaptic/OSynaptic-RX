/*
 * MultiSensorRX.ino -- OSynaptic-RX multi-sensor dispatcher (Arduino)
 * =====================================================================
 * Demonstrates receiving frames from multiple OSynaptic-TX nodes on a
 * single UART channel and dispatching each reading to a per-sensor
 * handler based on the device AID and sensor_id.
 *
 * Compatible boards: Arduino Uno, Nano, Mega, ESP8266, ESP32, STM32, etc.
 *
 * Scenario:
 *   Three TX nodes share one RS-485 bus (or one LoRa / UDP channel).
 *   Each identifies itself via the 'aid' field in the wire frame:
 *     Node A  aid = 0x00000001  sensors: T1 (Cel),  H1 (Pct)
 *     Node B  aid = 0x00000002  sensors: P1 (hPa)
 *     Node C  aid = 0x00000003  sensors: T1 (Cel)   -- different node, same sensor name
 *
 * The on_frame() callback dispatches to individual handler functions so
 * that the application logic for each node / sensor stays cleanly
 * separated.
 *
 * Memory (ATmega328P):
 *   OSRXParser static : 102 B  (buf[96] + len + fn_ptr + ctx_ptr)
 *   Stack peak        : ~55 B  (osrx_feed_done → parse → callback)
 *   Flash total       : ~760 B (full library, -Os)
 *
 * Wiring: packets received over Serial at 115200 baud.
 * Frame boundary: UART idle-gap of FRAME_TIMEOUT_MS milliseconds.
 */

#include <OSynaptic-RX.h>

/* ------------------------------------------------------------------
 * Known device AIDs
 * ------------------------------------------------------------------ */
#define AID_NODE_A  0x00000001UL
#define AID_NODE_B  0x00000002UL
#define AID_NODE_C  0x00000003UL

/* ------------------------------------------------------------------
 * Per-sensor handler prototypes
 * ------------------------------------------------------------------ */
static void handle_temp    (osrx_u32 aid, const osrx_sensor_field *f);
static void handle_humidity(osrx_u32 aid, const osrx_sensor_field *f);
static void handle_pressure(osrx_u32 aid, const osrx_sensor_field *f);
static void handle_unknown (osrx_u32 aid, const osrx_sensor_field *f);

/* ------------------------------------------------------------------
 * Parser state (file-scope = 0-initialised, 0 stack cost)
 * ------------------------------------------------------------------ */
static OSRXParser g_parser;

/* ------------------------------------------------------------------
 * Frame complete callback -- dispatches by AID + sensor_id
 * ------------------------------------------------------------------ */
static void on_frame(
    const osrx_packet_meta  *meta,
    const osrx_sensor_field *field,
    const osrx_u8           * /*raw*/,
    int                       /*raw_len*/,
    void                    * /*ctx*/)
{
    if (!meta->crc8_ok || !meta->crc16_ok) {
        Serial.println(F("[RX] CRC error -- frame discarded"));
        return;
    }
    if (!field) {
        /* Not a sensor/data body (wrong cmd byte or parse failure) */
        Serial.print(F("[RX] non-sensor frame cmd=0x"));
        Serial.println((unsigned)meta->cmd, HEX);
        return;
    }

    /* Dispatch by sensor_id string */
    if      (strcmp(field->sensor_id, "T1") == 0)   handle_temp    (meta->aid, field);
    else if (strcmp(field->sensor_id, "H1") == 0)   handle_humidity(meta->aid, field);
    else if (strcmp(field->sensor_id, "P1") == 0)   handle_pressure(meta->aid, field);
    else                                              handle_unknown (meta->aid, field);
}

/* ------------------------------------------------------------------
 * Helper: print integer-only value
 * ------------------------------------------------------------------ */
static void print_value(const osrx_sensor_field *f)
{
    osrx_i32 whole = f->scaled / (osrx_i32)OSRX_VALUE_SCALE;
    osrx_i32 frac  = f->scaled % (osrx_i32)OSRX_VALUE_SCALE;
    if (frac < 0L) { frac = -frac; }
    Serial.print((long)whole);
    Serial.print('.');
    if (frac < 1000L) Serial.print('0');
    if (frac < 100L)  Serial.print('0');
    if (frac < 10L)   Serial.print('0');
    Serial.print((long)frac);
    Serial.print(' ');
    Serial.print(f->unit);
}

/* ------------------------------------------------------------------
 * Per-sensor handlers
 * ------------------------------------------------------------------ */

static void handle_temp(osrx_u32 aid, const osrx_sensor_field *f)
{
    Serial.print(F("TEMP  aid="));
    Serial.print((unsigned long)aid, HEX);
    Serial.print(F("  "));
    print_value(f);
    Serial.println();

    /*
     * Node-specific logic example:
     * if (aid == AID_NODE_A && f->scaled > 350000L) {
     *     Serial.println(F("  [WARN] Node A overtemp!"));
     * }
     */
}

static void handle_humidity(osrx_u32 aid, const osrx_sensor_field *f)
{
    Serial.print(F("HUM   aid="));
    Serial.print((unsigned long)aid, HEX);
    Serial.print(F("  "));
    print_value(f);
    Serial.println();
}

static void handle_pressure(osrx_u32 aid, const osrx_sensor_field *f)
{
    Serial.print(F("PRES  aid="));
    Serial.print((unsigned long)aid, HEX);
    Serial.print(F("  "));
    print_value(f);
    Serial.println();
}

static void handle_unknown(osrx_u32 aid, const osrx_sensor_field *f)
{
    Serial.print(F("[RX] unknown sensor id="));
    Serial.print(f->sensor_id);
    Serial.print(F("  aid="));
    Serial.println((unsigned long)aid, HEX);
}

/* ------------------------------------------------------------------
 * Idle-gap tracking
 * ------------------------------------------------------------------ */
#define BAUD_RATE        115200UL
#define FRAME_TIMEOUT_MS 15

static unsigned long s_last_byte_ms = 0UL;
static int           s_got_byte     = 0;

/* ------------------------------------------------------------------
 * setup() / loop()
 * ------------------------------------------------------------------ */
void setup(void)
{
    Serial.begin(BAUD_RATE);
    while (!Serial) { ; }
    osrx_parser_init(&g_parser, on_frame, 0);
    Serial.println(F("OSynaptic-RX MultiSensorRX ready"));
}

void loop(void)
{
    while (Serial.available() > 0) {
        osrx_u8 b = (osrx_u8)Serial.read();
        osrx_feed_byte(&g_parser, b);
        s_last_byte_ms = millis();
        s_got_byte     = 1;
    }

    if (s_got_byte && (millis() - s_last_byte_ms) >= FRAME_TIMEOUT_MS) {
        osrx_feed_done(&g_parser);
        s_got_byte = 0;
    }
}
