/*
 * OSynaptic-RX  --  Basic RX Example
 * =====================================
 * Target: Any 8-bit MCU (e.g. ATmega328P @ 16 MHz, PIC16F, STM8, ...)
 * Toolchain: avr-gcc, sdcc, xc8, arm-none-eabi-gcc  (C89 mode)
 *
 * This example receives a raw wire packet over UART and decodes it into
 * a human-readable sensor reading using integer-only arithmetic (no float).
 * Replace uart_recv_byte() with your platform's low-level serial read.
 *
 * Build (avr-gcc, ATmega328P):
 *   avr-gcc -std=c89 -mmcu=atmega328p -Os -Wall \
 *     -I../include \
 *     ../src/osrx_crc.c ../src/osrx_b62.c \
 *     ../src/osrx_packet.c ../src/osrx_sensor.c \
 *     basic_rx.c -o basic_rx.elf
 */

#include "../include/osrx_config.h"
#include "../include/osrx_types.h"
#include "../include/osrx_sensor.h"

/* ----------------------------------------------------------------------- */
/* Platform stubs -- replace with your UART routines                       */
/* ----------------------------------------------------------------------- */

static int uart_recv_byte(osrx_u8 *out)
{
    /* Return 1 and fill *out when a byte is available; return 0 if none.
     * AVR example (blocking):
     *   while (!(UCSR0A & (1 << RXC0)));
     *   *out = UDR0;
     *   return 1;
     */
    (void)out;
    return 0;
}

/* Debug output stub -- replace with your serial/LCD write. */
static void debug_str(const char *s)
{
    (void)s;
}

/* ----------------------------------------------------------------------- */
/* Simple framing: accumulate bytes until OSTX_PACKET_MAX received.       */
/* For a real protocol you would use a length-prefix or delimiter.        */
/* ----------------------------------------------------------------------- */

static osrx_u8  g_rx_buf[OSRX_PACKET_MAX];
static int      g_rx_len = 0;

/* Returns 1 when a complete (fixed-max-length) frame has been received. */
static int rx_accumulate(void)
{
    osrx_u8 b;
    if (g_rx_len >= OSRX_PACKET_MAX) { g_rx_len = 0; }
    if (uart_recv_byte(&b)) {
        g_rx_buf[g_rx_len++] = b;
        if (g_rx_len == OSRX_PACKET_MAX) { return 1; }
    }
    return 0;
}

/* ----------------------------------------------------------------------- */
/* Application                                                              */
/* ----------------------------------------------------------------------- */

int main(void)
{
    osrx_packet_meta  meta;
    osrx_sensor_field field;
    osrx_i32          whole;
    osrx_i32          frac;

    while (1) {
        if (!rx_accumulate()) { continue; }

        if (osrx_sensor_recv(g_rx_buf, g_rx_len, &meta, &field)) {
            /*
             * Integer-only display of the decoded value.
             *   scaled = 215000  (21.50 Cel with scale 10000)
             *   whole  = 215000 / 10000 = 21
             *   frac   = 215000 % 10000 = 5000  --> "5000" (4 decimal places)
             *
             * On AVR, avoid printf() to save ~2 KB flash; use a small
             * itoa() or a custom digit-by-digit UART print instead.
             */
            whole = field.scaled / (osrx_i32)OSRX_VALUE_SCALE;
            frac  = field.scaled % (osrx_i32)OSRX_VALUE_SCALE;
            if (frac < 0) { frac = -frac; }

            /*
             * 'whole' and 'frac' are now plain integers ready to format.
             * Example: whole=21, frac=5000  --> print "21.5000 Cel"
             */
            (void)whole;
            (void)frac;
            debug_str(field.sensor_id);
            debug_str(field.unit);

            /* Reset buffer for next packet. */
            g_rx_len = 0;
        } else {
            /* CRC mismatch or parse error -- discard and try next byte. */
            g_rx_len = 0;
        }
    }

    return 0;
}
