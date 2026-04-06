#ifndef OSRX_B62_H
#define OSRX_B62_H

#include "osrx_types.h"
#include "osrx_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Decode a length-bounded Base62 string to a 32-bit signed integer.
 *
 * Alphabet: "0-9 a-z A-Z"  (matches OSynaptic-TX / OpenSynaptic Python).
 * Negative values are indicated by a leading '-'.
 *
 * Parameters:
 *   s   -- pointer to Base62 characters (need not be NUL-terminated)
 *   len -- number of significant characters in s
 *   ok  -- set to 1 on success, 0 on failure (may be NULL)
 *
 * Returns the decoded value on success; 0 on failure.
 */
osrx_i32 osrx_b62_decode(const char *s, int len, int *ok);

#ifdef __cplusplus
}
#endif

#endif /* OSRX_B62_H */
