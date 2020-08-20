#ifndef ADBLOCK_RUST_FFI_H
#define ADBLOCK_RUST_FFI_H

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct C_Engine C_Engine;

/**
 * An external callback that receives a hostname and two out-parameters for start and end
 * position. The callback should fill the start and end positions with the start and end indices
 * of the domain part of the hostname.
 */
typedef void (*C_DomainResolverCallback)(const char*, uint32_t*, uint32_t*);

/**
 * Destroy a `*c_char` once you are done with it.
 */
void c_char_buffer_destroy(char *s);

/**
 * Adds a resource to the engine by name
 */
void engine_add_resource(C_Engine *engine,
                         const char *key,
                         const char *content_type,
                         const char *data);

/**
 * Adds a list of `Resource`s from JSON format
 */
void engine_add_resources(C_Engine *engine, const char *resources);

/**
 * Adds a tag to the engine for consideration
 */
void engine_add_tag(C_Engine *engine, const char *tag);

/**
 * Create a new `Engine`.
 */
C_Engine *engine_create(const char *rules);

/**
 * Deserializes a previously serialized data file list.
 */
bool engine_deserialize(C_Engine *engine, const char *data, size_t data_size);

/**
 * Destroy a `Engine` once you are done with it.
 */
void engine_destroy(C_Engine *engine);

/**
 * Returns a stylesheet containing all generic cosmetic rules that begin with any of the provided class and id selectors
 *
 * The leading '.' or '#' character should not be provided
 */
char *engine_hidden_class_id_selectors(C_Engine *engine,
                                       const char *const *classes,
                                       size_t classes_size,
                                       const char *const *ids,
                                       size_t ids_size,
                                       const char *const *exceptions,
                                       size_t exceptions_size);

/**
 * Checks if a `url` matches for the specified `Engine` within the context.
 */
bool engine_match(C_Engine *engine,
                  const char *url,
                  const char *host,
                  const char *tab_host,
                  bool third_party,
                  const char *resource_type,
                  bool *explicit_cancel,
                  bool *saved_from_exception,
                  char **redirect);

/**
 * Removes a tag to the engine for consideration
 */
void engine_remove_tag(C_Engine *engine, const char *tag);

/**
 * Checks if a tag exists in the engine
 */
bool engine_tag_exists(C_Engine *engine, const char *tag);

/**
 * Returns a set of cosmetic filtering resources specific to the given url, in JSON format
 */
char *engine_url_cosmetic_resources(C_Engine *engine, const char *url);

/**
 * Passes a callback to the adblock library, allowing it to be used for domain resolution.
 *
 * This is required to be able to use any adblocking functionality.
 *
 * Returns true on success, false if a callback was already set previously.
 */
bool set_domain_resolver(C_DomainResolverCallback resolver);

#endif /* ADBLOCK_RUST_FFI_H */
