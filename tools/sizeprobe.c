#include "../include/osrx_config.h"
#include "../include/osrx_types.h"
#include "../include/osrx_packet.h"
#include "../include/osrx_sensor.h"
#include "../include/osrx_parser.h"
#include <stdio.h>

int main(void)
{
    printf("--- struct sizes (host pointer size = %u B) ---\n",
           (unsigned)sizeof(void *));
    printf("osrx_packet_meta   = %u B\n", (unsigned)sizeof(osrx_packet_meta));
    printf("osrx_sensor_field  = %u B\n", (unsigned)sizeof(osrx_sensor_field));
    printf("OSRXParser         = %u B  (buf=%u + len=4 + ptr=2x%u)\n",
           (unsigned)sizeof(OSRXParser),
           (unsigned)OSRX_PACKET_MAX,
           (unsigned)sizeof(void *));
    printf("\n--- config knobs ---\n");
    printf("OSRX_PACKET_MAX  = %d\n", OSRX_PACKET_MAX);
    printf("OSRX_BODY_MAX    = %d\n", OSRX_BODY_MAX);
    printf("OSRX_ID_MAX      = %d\n", OSRX_ID_MAX);
    printf("OSRX_UNIT_MAX    = %d\n", OSRX_UNIT_MAX);
    printf("OSRX_B62_MAX     = %d\n", OSRX_B62_MAX);
    printf("OSRX_VALUE_SCALE = %ld\n", (long)OSRX_VALUE_SCALE);
    return 0;
}
