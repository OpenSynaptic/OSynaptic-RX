/*
 * ESP32UdpRX.ino -- OSynaptic-RX UDP receiver example (ESP32)
 * ============================================================
 * Hardware: ESP32 (any variant)
 * Transport: Wi-Fi / UDP (same IP + port as the OpenSynaptic Python hub)
 * Protocol: OpenSynaptic wire format v1 (FULL frame)
 *
 * Each UDP datagram contains exactly one OpenSynaptic wire frame, so
 * the parser callback is driven via osrx_feed_bytes() which calls
 * osrx_feed_done() internally.
 */

#include <WiFi.h>
#include <WiFiUdp.h>
#include <OSynaptic-RX.h>

/* ---------- Wi-Fi credentials (edit before flashing) -------------- */
static const char WIFI_SSID[] = "YourSSID";
static const char WIFI_PASS[] = "YourPassword";
/* ------------------------------------------------------------------ */

#define UDP_LISTEN_PORT  5005

static WiFiUDP    g_udp;
static OSRXParser g_parser;

/* ------------------------------------------------------------------ */
/* Frame callback                                                      */
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
        Serial.println("[RX] CRC error — frame discarded");
        return;
    }
    if (!field) {
        Serial.printf("[RX] cmd=0x%02X aid=%08lX (no sensor body)\r\n",
                      (unsigned)meta->cmd, (unsigned long)meta->aid);
        return;
    }

    osrx_i32 whole = field->scaled / (osrx_i32)OSRX_VALUE_SCALE;
    osrx_i32 frac  = field->scaled % (osrx_i32)OSRX_VALUE_SCALE;
    if (frac < 0L) { frac = -frac; }

    Serial.printf("[RX] aid=%08lX ts=%lu  %s = %ld.%04ld %s\r\n",
        (unsigned long)meta->aid,
        (unsigned long)meta->ts_sec,
        field->sensor_id,
        (long)whole,
        (long)frac,
        field->unit);
}

/* ------------------------------------------------------------------ */
void setup(void)
{
    Serial.begin(115200);
    Serial.println("OSynaptic-RX / ESP32 UDP starting...");

    WiFi.begin(WIFI_SSID, WIFI_PASS);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print('.');
    }
    Serial.printf("\r\nConnected: %s\r\n", WiFi.localIP().toString().c_str());

    g_udp.begin(UDP_LISTEN_PORT);
    Serial.printf("Listening on UDP port %d\r\n", UDP_LISTEN_PORT);

    osrx_parser_init(&g_parser, on_frame, 0);
}

void loop(void)
{
    int packet_size = g_udp.parsePacket();
    if (packet_size > 0) {
        static osrx_u8 udp_buf[OSRX_PACKET_MAX];
        int n = g_udp.read((char *)udp_buf,
                           packet_size < OSRX_PACKET_MAX
                               ? packet_size
                               : OSRX_PACKET_MAX);
        if (n > 0) {
            osrx_feed_bytes(&g_parser, udp_buf, n);
        }
    }
}
