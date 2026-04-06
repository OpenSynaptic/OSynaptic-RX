/*
 * BareMetalUARTRX.ino -- OSynaptic-RX on ATmega328P without Arduino Serial
 * =========================================================================
 * Shows osrx_feed_byte() driven by direct USART0 hardware register polling.
 * No Serial / Stream overhead. Purpose:
 *
 *   - Demonstrate absolute-minimum integration: ~9 statements in loop()
 *   - Confirm the library is usable in strict C89/C99 embedded contexts
 *   - Provide a template for IAR, Microchip XC8, SDCC targets
 *
 * On non-AVR boards replace the uart_init() / uart_rx_ready() /
 * uart_read_byte() stubs with your MCU's UART register equivalents.
 *
 * Compatible boards: Arduino Uno, Nano, Pro Mini (ATmega328P @ 5 V/16 MHz).
 *
 * Memory (ATmega328P):
 *   OSRXParser static : 102 B  (buf[96] + len + fn_ptr + ctx_ptr)
 *   Stack peak        : ~12 B  (loop + osrx_feed_byte)
 *   Stack peak (cb)   : ~55 B  (osrx_feed_done → parse → callback)
 *   Flash total       : ~760 B (full library, -Os / MinSizeRel)
 *
 * Compare with BasicRX.ino which uses Serial.available():
 *   Serial.begin() alone pulls in ~400 B of Stream/Print overhead.
 */

#include <avr/io.h>
#include <OSynaptic-RX.h>

/* ------------------------------------------------------------------
 * ATmega328P USART0 bare-metal initialiser (RX only)
 * UBRR = F_CPU / (16 * baud) - 1 = 16000000 / (16 * 9600) - 1 = 103
 * ------------------------------------------------------------------ */
static void uart_init(void)
{
    UBRR0H = 0;
    UBRR0L = 103u;                   /* 9600 baud @ 16 MHz             */
    UCSR0B = (1u << RXEN0);          /* enable RX only                 */
    UCSR0C = (1u << UCSZ01) | (1u << UCSZ00); /* 8-N-1               */
}

/* Returns 1 if a byte is waiting in UDR0. */
static int uart_rx_ready(void)
{
    return (int)(UCSR0A & (1u << RXC0));
}

/* Read one byte from USART0 (call only when uart_rx_ready() is non-zero). */
static osrx_u8 uart_read_byte(void)
{
    return (osrx_u8)UDR0;
}

/* ------------------------------------------------------------------
 * Frame complete callback
 * millis() is still functional because Arduino's timer setup runs
 * before setup(); only the Serial class is bypassed.
 * ------------------------------------------------------------------ */
static void on_frame(
    const osrx_packet_meta  *meta,
    const osrx_sensor_field *field,
    const osrx_u8           * /*raw*/,
    int                       /*raw_len*/,
    void                    * /*ctx*/)
{
    /* Discard on CRC error or missing sensor body */
    if (!meta->crc8_ok || !meta->crc16_ok) return;
    if (!field) return;

    /*
     * Application logic here.
     * Example: drive a status LED based on temperature threshold.
     *   if (field->scaled > 300000L)   PORTB |= _BV(PB5);  // > 30 °C
     *   else                           PORTB &= ~_BV(PB5);
     *
     * Integer-only real value:
     *   whole = field->scaled / OSRX_VALUE_SCALE
     *   frac  = field->scaled % OSRX_VALUE_SCALE (always positive after abs)
     */
    (void)field;   /* suppress unused-variable warning in this template */
}

/* ------------------------------------------------------------------
 * Parser state (file-scope = 0-initialised, 0 stack cost).
 * ------------------------------------------------------------------ */
static OSRXParser g_parser;

/* ------------------------------------------------------------------
 * Idle-gap tracking for UART frame-boundary detection.
 * 15 ms accounts for a 9600-baud inter-frame gap at the receiver.
 * ------------------------------------------------------------------ */
#define FRAME_TIMEOUT_MS 15u

static unsigned long s_last_byte_ms = 0UL;
static int           s_got_byte     = 0;

/* ------------------------------------------------------------------
 * setup()
 * ------------------------------------------------------------------ */
void setup(void)
{
    uart_init();
    osrx_parser_init(&g_parser, on_frame, 0);
}

/* ------------------------------------------------------------------
 * loop() -- polling receive loop; no Arduino Serial overhead.
 * ------------------------------------------------------------------ */
void loop(void)
{
    /* Drain any waiting bytes from USART0 hardware buffer */
    if (uart_rx_ready()) {
        osrx_u8 b = uart_read_byte();
        osrx_feed_byte(&g_parser, b);
        s_last_byte_ms = millis();
        s_got_byte     = 1;
    }

    /* UART idle gap → end of frame */
    if (s_got_byte && (millis() - s_last_byte_ms) >= FRAME_TIMEOUT_MS) {
        osrx_feed_done(&g_parser);
        s_got_byte = 0;
    }
}
