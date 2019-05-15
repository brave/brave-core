#ifndef ADBLOCK_RUST_FFI_H
#define ADBLOCK_RUST_FFI_H

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct C_Engine C_Engine;

/**
 * Create a new `Engine`.
 */
C_Engine *engine_create(const char *rules);

/**
 * Deserializes a previously serialized data file list.
 */
bool engine_deserialize(C_Engine *engine, const char *data);

/**
 * Destroy a `Engine` once you are done with it.
 */
void engine_destroy(C_Engine *engine);

/**
 * Checks if a `url` matches for the specified `Engine` within the context.
 */
bool engine_match(C_Engine *engine,
                  const char *url,
                  const char *host,
                  const char *tab_host,
                  bool third_party,
                  const char *resource_type);

#endif /* ADBLOCK_RUST_FFI_H */
