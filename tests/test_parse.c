/*
 * tests/test_parse.c -- Known-answer tests for OSynaptic-RX (C89)
 *
 * Build (standalone):
 *   gcc -std=c89 -Wall -Wextra -pedantic \
 *     -I../include \
 *     ../src/osrx_crc.c ../src/osrx_b62.c \
 *     ../src/osrx_packet.c ../src/osrx_sensor.c \
 *     ../src/osrx_parser.c \
 *     test_parse.c -o test_parse
 *   ./test_parse
 *
 * Built automatically by CMake if OSRX_BUILD_TESTS=ON.
 *
 * Exit code: 0 = all passed, 1 = at least one failure.
 */

#include <stdio.h>
#include <string.h>
#include "osrx_crc.h"
#include "osrx_b62.h"
#include "osrx_packet.h"
#include "osrx_sensor.h"
#include "osrx_parser.h"

/* ------------------------------------------------------------------ */
/* Minimal test harness                                                */
/* ------------------------------------------------------------------ */

static int g_pass = 0;
static int g_fail = 0;

#define CHECK(cond)                                                    \
    do {                                                               \
        if (cond) {                                                    \
            printf("  PASS: %s\n", #cond);                            \
            ++g_pass;                                                  \
        } else {                                                       \
            printf("  FAIL: %s  (line %d)\n", #cond, __LINE__);       \
            ++g_fail;                                                  \
        }                                                              \
    } while (0)

/* ------------------------------------------------------------------ */
/* CRC known-answer tests                                              */
/* ------------------------------------------------------------------ */

#define CRC8_CHECK_VAL  0xF4u
#define CRC16_CHECK_VAL 0x29B1u

static void test_crc(void)
{
    static const osrx_u8 data[] = "123456789";
    osrx_u8  c8;
    osrx_u16 c16;

    printf("--- CRC ---\n");

    c8  = osrx_crc8 (data, 9, 0x07u, 0x00u);
    c16 = osrx_crc16(data, 9, 0x1021u, 0xFFFFu);

    CHECK(c8  == CRC8_CHECK_VAL);
    CHECK(c16 == CRC16_CHECK_VAL);

    /* Edge: NULL input or zero length returns 0 */
    CHECK(osrx_crc8 (0, 0, 0x07u, 0x00u)    == 0u);
    CHECK(osrx_crc16(0, 0, 0x1021u, 0xFFFFu) == 0u);
}

/* ------------------------------------------------------------------ */
/* Base62 known-answer tests                                           */
/* ------------------------------------------------------------------ */

static void test_b62(void)
{
    int      ok;
    osrx_i32 v;

    printf("--- Base62 ---\n");

    /* Zero */
    v = osrx_b62_decode("0", 1, &ok);
    CHECK(ok == 1 && v == 0);

    /* Positive small */
    v = osrx_b62_decode("10", 2, &ok);
    CHECK(ok == 1 && v == 62);

    /* Negative */
    v = osrx_b62_decode("-1", 2, &ok);
    CHECK(ok == 1 && v == -1);

    /* 215000 = 55*62^2 + 57*62 + 46
     * index 55 -> 'T', index 57 -> 'V', index 46 -> 'K'  => "TVK" */
    v = osrx_b62_decode("TVK", 3, &ok);
    CHECK(ok == 1 && v == 215000L);

    /* Empty / invalid */
    v = osrx_b62_decode("", 0, &ok);
    CHECK(ok == 0);

    v = osrx_b62_decode("!bad", 4, &ok);
    CHECK(ok == 0);

    /* NULL */
    v = osrx_b62_decode((const char *)0, 0, &ok);
    CHECK(ok == 0 && v == 0);
}

/* ------------------------------------------------------------------ */
/* Build a test wire frame matching the TX format                     */
/* ------------------------------------------------------------------ */

/*
 * Manually construct:
 *   cmd=63, route=1, aid=0x01020304, tid=7
 *   ts=0x00000000 00001234 (bytes [7..12]=0,0,0,0,0x12,0x34)
 *   body = "T1|Cel|TVq"  (sensor_id="T1", unit="Cel", b62="TVq"=215000)
 *
 * Wire:
 *   [0]=63 [1]=1 [2..5]=aid BE [6]=tid [7..12]=ts [13..22]=body
 *   [23]=crc8(body) [24..25]=crc16(frame[0..23])
 */

#define TEST_AID     0x01020304UL
#define TEST_TID     7
#define TEST_TSSEC   0x00001234UL
#define TEST_CMD     63
#define TEST_ROUTE   1

/*
 * Body uses the current OpenSynaptic wire format:
 *   "{aid}.U.{ts_b64}|{sid}>U.{unit}:{b62}|"
 * aid  = 0x01020304 = 16909060 (decimal)
 * ts   = 0x00001234 -> 6-byte BE -> base64url "AAAAABI0"
 * sid  = "T1", unit-code = "A01" (degree Celsius), b62 = "TVK" (215000)
 */
static const char TEST_BODY[] = "16909060.U.AAAAABI0|T1>U.A01:TVK|";
#define TEST_BODY_LEN 33

static int build_frame(osrx_u8 *out, int out_cap)
{
    osrx_u8  *p  = out;
    osrx_u8   c8;
    osrx_u16  c16;
    int       total;

    total = 13 + TEST_BODY_LEN + 3;
    if (total > out_cap) { return 0; }

    p[0]  = (osrx_u8)TEST_CMD;
    p[1]  = (osrx_u8)TEST_ROUTE;
    p[2]  = (osrx_u8)((TEST_AID >> 24) & 0xFFu);
    p[3]  = (osrx_u8)((TEST_AID >> 16) & 0xFFu);
    p[4]  = (osrx_u8)((TEST_AID >>  8) & 0xFFu);
    p[5]  = (osrx_u8)( TEST_AID        & 0xFFu);
    p[6]  = (osrx_u8)TEST_TID;
    p[7]  = 0;
    p[8]  = 0;
    p[9]  = 0;
    p[10] = 0;
    p[11] = (osrx_u8)((TEST_TSSEC >>  8) & 0xFFu);
    p[12] = (osrx_u8)( TEST_TSSEC        & 0xFFu);
    memcpy(p + 13, TEST_BODY, (size_t)TEST_BODY_LEN);

    c8 = osrx_crc8(p + 13, TEST_BODY_LEN, 0x07u, 0x00u);
    p[13 + TEST_BODY_LEN] = c8;

    c16 = osrx_crc16(p, 13 + TEST_BODY_LEN + 1, 0x1021u, 0xFFFFu);
    p[13 + TEST_BODY_LEN + 1] = (osrx_u8)(c16 >> 8);
    p[13 + TEST_BODY_LEN + 2] = (osrx_u8)(c16 & 0xFFu);

    return total;
}

/* ------------------------------------------------------------------ */
/* Packet decode tests                                                 */
/* ------------------------------------------------------------------ */

static void test_packet_decode(void)
{
    osrx_u8          frame[96];
    osrx_packet_meta meta;
    int              flen;
    int              rc;

    printf("--- packet_decode ---\n");

    flen = build_frame(frame, (int)sizeof(frame));
    CHECK(flen == 49);

    rc = osrx_packet_decode(frame, flen, &meta);
    CHECK(rc == 1);
    CHECK(meta.cmd         == (osrx_u8)TEST_CMD);
    CHECK(meta.aid         == (osrx_u32)TEST_AID);
    CHECK(meta.tid         == (osrx_u8)TEST_TID);
#if !OSRX_NO_TIMESTAMP
    CHECK(meta.ts_sec      == (osrx_u32)TEST_TSSEC);
#endif
    CHECK(meta.body_off    == 13);
    CHECK(meta.body_len    == TEST_BODY_LEN);
    CHECK(meta.crc8_ok     == 1);
    CHECK(meta.crc16_ok    == 1);

    /* Too short */
    rc = osrx_packet_decode(frame, 15, &meta);
    CHECK(rc == 0);

    /* NULL */
    rc = osrx_packet_decode((const osrx_u8 *)0, flen, &meta);
    CHECK(rc == 0);

    /* Corrupted CRC-8 byte */
    frame[46] ^= 0x01u;
    rc = osrx_packet_decode(frame, flen, &meta);
    CHECK(rc == 1 && meta.crc8_ok == 0);
    frame[46] ^= 0x01u; /* restore */

#if OSRX_VALIDATE_CRC16
    /* Corrupted CRC-16 */
    frame[48] ^= 0xFFu;
    rc = osrx_packet_decode(frame, flen, &meta);
    CHECK(rc == 1 && meta.crc16_ok == 0);
    frame[48] ^= 0xFFu; /* restore */
#endif
}

/* ------------------------------------------------------------------ */
/* Sensor unpack tests                                                 */
/* ------------------------------------------------------------------ */

static void test_sensor_unpack(void)
{
    osrx_u8           frame[96];
    osrx_packet_meta  meta;
    osrx_sensor_field field;
    int               flen;
    int               rc;

    printf("--- sensor_unpack ---\n");

    flen = build_frame(frame, (int)sizeof(frame));
    rc   = osrx_packet_decode(frame, flen, &meta);
    CHECK(rc == 1 && meta.crc8_ok == 1 && meta.crc16_ok == 1);

    rc = osrx_sensor_unpack(frame + meta.body_off, meta.body_len, &field);
    CHECK(rc == 1);
    CHECK(strcmp(field.sensor_id, "T1")  == 0);
    CHECK(strcmp(field.unit,      "A01") == 0);
    CHECK(field.scaled == 215000L);

    /* Convenience call */
    {
        osrx_packet_meta  meta2;
        osrx_sensor_field field2;
        flen = build_frame(frame, (int)sizeof(frame));
        rc   = osrx_sensor_recv(frame, flen, &meta2, &field2);
        CHECK(rc == 1);
        CHECK(field2.scaled == 215000L);
    }
}

/* ------------------------------------------------------------------ */
/* Streaming parser tests (only when parser is compiled in)           */
/* ------------------------------------------------------------------ */

#ifndef OSRX_NO_PARSER

static int          g_cb_called;
static osrx_i32     g_cb_scaled;
static int          g_cb_crc_ok;

static void parser_cb(
    const osrx_packet_meta  *meta,
    const osrx_sensor_field *field,
    const osrx_u8           *raw,
    int                      raw_len,
    void                    *ctx)
{
    (void)raw; (void)raw_len; (void)ctx;
    g_cb_called = 1;
    g_cb_crc_ok = meta->crc8_ok && meta->crc16_ok;
    g_cb_scaled = field ? field->scaled : 0;
}

static void test_parser(void)
{
    osrx_u8    frame[96];
    OSRXParser p;
    int        flen;
    int        rc;

    printf("--- osrx_parser ---\n");

    flen = build_frame(frame, (int)sizeof(frame));

    /* feed_bytes: fires callback, returns 1 */
    g_cb_called = 0; g_cb_scaled = 0; g_cb_crc_ok = 0;
    osrx_parser_init(&p, parser_cb, (void *)0);
    rc = osrx_feed_bytes(&p, frame, flen);
    CHECK(rc == 1);
    CHECK(g_cb_called == 1);
    CHECK(g_cb_crc_ok == 1);
    CHECK(g_cb_scaled == 215000L);

    /* byte-by-byte */
    {
        int i;
        g_cb_called = 0;
        osrx_parser_init(&p, parser_cb, (void *)0);
        for (i = 0; i < flen; ++i) {
            osrx_feed_byte(&p, frame[i]);
        }
        rc = osrx_feed_done(&p);
        CHECK(rc == 1 && g_cb_called == 1 && g_cb_scaled == 215000L);
    }

    /* overflow: push > OSRX_PACKET_MAX bytes then feed_done */
    {
        int i;
        g_cb_called = 0;
        osrx_parser_init(&p, parser_cb, (void *)0);
        for (i = 0; i < OSRX_PACKET_MAX + 1; ++i) {
            osrx_feed_byte(&p, (osrx_u8)0x00);
        }
        /* After overflow reset, buffer is empty; feed_done returns 0 */
        rc = osrx_feed_done(&p);
        CHECK(rc == 0 && g_cb_called == 0);
    }

    /* reset discards without firing callback */
    {
        g_cb_called = 0;
        osrx_parser_init(&p, parser_cb, (void *)0);
        osrx_feed_byte(&p, frame[0]);
        osrx_parser_reset(&p);
        rc = osrx_feed_done(&p);
        CHECK(rc == 0 && g_cb_called == 0);
    }
}

#endif /* OSRX_NO_PARSER */

/* ------------------------------------------------------------------ */
/* Entry point                                                         */
/* ------------------------------------------------------------------ */

int main(void)
{
    printf("=== OSynaptic-RX unit tests ===\n");
    test_crc();
    test_b62();
    test_packet_decode();
    test_sensor_unpack();
#ifndef OSRX_NO_PARSER
    test_parser();
#endif
    printf("==============================\n");
    printf("Total: %d passed, %d failed\n", g_pass, g_fail);
    return (g_fail == 0) ? 0 : 1;
}
