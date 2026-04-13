#include <stddef.h>
#include <stdio.h>

#include "osrx_packet.h"
#include "osrx_sensor.h"
#include "osrx_types.h"

typedef char osrx_assert_u32_is_4_bytes[(sizeof(osrx_u32) == 4u) ? 1 : -1];
typedef char osrx_assert_i32_is_4_bytes[(sizeof(osrx_i32) == 4u) ? 1 : -1];

static int g_pass = 0;
static int g_fail = 0;

#define CHECK(cond)                                                    \
    do {                                                               \
        if (cond) {                                                    \
            printf("  PASS: %s\n", #cond);                           \
            ++g_pass;                                                  \
        } else {                                                       \
            printf("  FAIL: %s  (line %d)\n", #cond, __LINE__);      \
            ++g_fail;                                                  \
        }                                                              \
    } while (0)

static void test_integer_widths(void)
{
    osrx_sensor_field field;
    osrx_packet_meta meta;

    printf("--- host integer widths ---\n");
    CHECK(sizeof(osrx_u32) == 4u);
    CHECK(sizeof(osrx_i32) == 4u);
    CHECK(sizeof(field.scaled) == 4u);
    CHECK(sizeof(meta.aid) == 4u);
#if !OSRX_NO_TIMESTAMP
    CHECK(sizeof(meta.ts_sec) == 4u);
#endif
}

static void test_host_layout(void)
{
    printf("--- host ABI layout ---\n");
    CHECK(offsetof(osrx_sensor_field, scaled) == 20u);
    CHECK(sizeof(osrx_sensor_field) == 24u);
#if OSRX_NO_TIMESTAMP
    CHECK(sizeof(osrx_packet_meta) == 28u);
#else
    CHECK(sizeof(osrx_packet_meta) == 32u);
#endif
}

int main(void)
{
    test_integer_widths();
    test_host_layout();

    printf("\nsummary: %d passed, %d failed\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}