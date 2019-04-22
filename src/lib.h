#ifndef ADBLOCK_RUST_FFI_H
#define ADBLOCK_RUST_FFI_H

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct C_Blocker C_Blocker;

/**
 * Create a new `Blocker`.
 */
C_Blocker *blocker_create(const char *rules);

/**
 * Destroy a `Blocker` once you are done with it.
 */
void blocker_destroy(C_Blocker *blocker);

/**
 * Checks if a `url` matches for the specified `Blocker` within the context.
 */
bool blocker_match(C_Blocker *blocker,
                   const char *url,
                   const char *tab_url,
                   const char *resource_type);

#endif /* ADBLOCK_RUST_FFI_H */
