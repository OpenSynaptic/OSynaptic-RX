#ifndef OSRX_B62_H
#define OSRX_B62_H

/*
 * osrx_b62.h -- Base62 decoder for OSynaptic-RX (C89).
 *
 * Decodes the value field of an OpenSynaptic wire frame back into an
 * int32.  The alphabet MUST match the encoder used on the TX side:
 *
 *   "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
 *    index 0-9  10-35  36-61
 *
 * Negative values are prefixed with '-'.
 * INT32_MIN is encoded by the TX as "-2lkCB2".
 */

#include "osrx_types.h"
#include "osrx_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * osrx_b62_decode() -- decode a length-bounded Base62 string to int32.
 *
 *   s   -- pointer to Base62 characters (need not be NUL-terminated)
 *   len -- number of characters to read (pass 0 or negative for error)
 *   ok  -- set to 1 on success, 0 on error (may be NULL)
 *
 * Returns the decoded value on success; 0 on error:
 *   - s is NULL, len <= 0
 *   - an invalid character is encountered (not in alphabet and not '-')
 */
osrx_i32 osrx_b62_decode(const char *s, int len, int *ok);

#ifdef __cplusplus
}
#endif

#endif /* OSRX_B62_H */
