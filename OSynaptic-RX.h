/*
 * OSynaptic-RX.h -- Arduino root entry header.
 *
 * Sketch usage:
 *   #include <OSynaptic-RX.h>
 *
 * The Arduino IDE resolves this header from the library root, then adds
 * src/ to the include path so all osrx_*.h headers become available.
 */

#ifndef OSYNAPTIC_RX_H
#define OSYNAPTIC_RX_H

#include "src/osrx_config.h"
#include "src/osrx_types.h"
#include "src/osrx_crc.h"
#include "src/osrx_b62.h"
#include "src/osrx_packet.h"
#include "src/osrx_sensor.h"
#include "src/osrx_units.h"
#if !OSRX_NO_PARSER
#  include "src/osrx_parser.h"
#endif

#endif /* OSYNAPTIC_RX_H */
