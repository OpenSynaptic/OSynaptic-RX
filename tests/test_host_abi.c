#include <stddef.h>
#include <stdio.h>

#include "osrx_packet.h"
#include "osrx_sensor.h"
#include "osrx_types.h"

typedef char osrx_assert_u32_is_4_bytes[(sizeof(osrx_u32) == 4u) ? 1 : -1];
typedef char osrx_assert_i32_is_4_bytes[(sizeof(osrx_i32) == 4u) ? 1 : -1];

typedef struct osrx_align_i32_probe {
    char     pad;
    osrx_i32 value;
} osrx_align_i32_probe;

static int g_pass = 0;
static int g_fail = 0;

#define OSRX_PUBLISHED_ID_MAX   9
#define OSRX_PUBLISHED_UNIT_MAX 9

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

static size_t round_up_size(size_t value, size_t alignment)
{
    return ((value + alignment - (size_t)1u) / alignment) * alignment;
}

static size_t i32_alignment(void)
{
    return offsetof(osrx_align_i32_probe, value);
}

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
    size_t field_align = i32_alignment();
    size_t field_bytes = (size_t)OSRX_ID_MAX + (size_t)OSRX_UNIT_MAX;
    size_t scaled_off = round_up_size(field_bytes, field_align);
    size_t field_size = round_up_size(
        scaled_off + sizeof(((osrx_sensor_field *)0)->scaled),
        field_align
    );

    printf("--- host ABI layout ---\n");
    if (OSRX_ID_MAX == OSRX_PUBLISHED_ID_MAX &&
        OSRX_UNIT_MAX == OSRX_PUBLISHED_UNIT_MAX) {
        CHECK(offsetof(osrx_sensor_field, scaled) == (size_t)20u);
        CHECK(sizeof(osrx_sensor_field) == (size_t)24u);
    } else {
        CHECK(offsetof(osrx_sensor_field, scaled) == scaled_off);
        CHECK(sizeof(osrx_sensor_field) == field_size);
    }
#if OSRX_NO_TIMESTAMP
    CHECK(sizeof(osrx_packet_meta) == (size_t)28u);
#else
    CHECK(sizeof(osrx_packet_meta) == (size_t)32u);
#endif
}

int main(void)
{
    test_integer_widths();
    test_host_layout();

    printf("\nsummary: %d passed, %d failed\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}