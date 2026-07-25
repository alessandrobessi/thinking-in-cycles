#ifndef CYCLELAB_JSONOUT_H
#define CYCLELAB_JSONOUT_H

#include <stdio.h>

/* Writes s to f as the contents of a JSON string (without the surrounding
 * quotes), escaping '"', '\\', and control characters. cyclelab's output
 * schema is fixed and known in advance, so modes compose JSON directly
 * with fprintf and use this only for the handful of fields that come from
 * the environment (hostname, kernel string) and could in principle contain
 * characters that need escaping. */
void json_write_escaped(FILE *f, const char *s);

#endif /* CYCLELAB_JSONOUT_H */
