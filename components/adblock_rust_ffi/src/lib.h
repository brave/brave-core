/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_ADBLOCK_RUST_FFI_SRC_LIB_H_
#define BRAVE_COMPONENTS_ADBLOCK_RUST_FFI_SRC_LIB_H_

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

/**
 * Main adblocking engine that allows efficient querying of resources to block.
 */
typedef struct C_Engine C_Engine;

/**
 * Includes information about any "special comments" as described by
 * https://help.eyeo.com/adblockplus/how-to-write-filters#special-comments
 */
typedef struct C_FilterListMetadata C_FilterListMetadata;

/**
 * An external callback that receives a hostname and two out-parameters for
 * start and end position. The callback should fill the start and end positions
 * with the start and end indices of the domain part of the hostname.
 */
typedef void (*C_DomainResolverCallback)(const char*, uint32_t*, uint32_t*);

/**
 * Passes a callback to the adblock library, allowing it to be used for domain
 * resolution.
 *
 * This is required to be able to use any adblocking functionality.
 *
 * Returns true on success, false if a callback was already set previously.
 */
bool set_domain_resolver(C_DomainResolverCallback resolver);

/**
 * Create a new `Engine`, interpreting `data` as a C string and then parsing as
 * a filter list in ABP syntax.
 */
struct C_Engine* engine_create_from_buffer(const char* data, size_t data_size);

/**
 * Create a new `Engine`, interpreting `data` as a C string and then parsing as
 * a filter list in ABP syntax. Also populates metadata from the filter list
 * into `metadata`.
 */
struct C_Engine* engine_create_from_buffer_with_metadata(
    const char* data,
    size_t data_size,
    struct C_FilterListMetadata** metadata);

/**
 * Create a new `Engine`, interpreting `rules` as a null-terminated C string and
 * then parsing as a filter list in ABP syntax.
 */
struct C_Engine* engine_create(const char* rules);

/**
 * Create a new `Engine`, interpreting `rules` as a null-terminated C string and
 * then parsing as a filter list in ABP syntax. Also populates metadata from the
 * filter list into `metadata`.
 */
struct C_Engine* engine_create_with_metadata(
    const char* rules,
    struct C_FilterListMetadata** metadata);

/**
 * Checks if a `url` matches for the specified `Engine` within the context.
 *
 * This API is designed for multi-engine use, so block results are used both as
 * inputs and outputs. They will be updated to reflect additional checking
 * within this engine, rather than being replaced with results just for this
 * engine.
 */
void engine_match(struct C_Engine* engine,
                  const char* url,
                  const char* host,
                  const char* tab_host,
                  bool third_party,
                  const char* resource_type,
                  bool* did_match_rule,
                  bool* did_match_exception,
                  bool* did_match_important,
                  char** filter,
                  char** redirect);

/**
 * Returns any CSP directives that should be added to a subdocument or document
 * request's response headers.
 */
char* engine_get_csp_directives(struct C_Engine* engine,
                                const char* url,
                                const char* host,
                                const char* tab_host,
                                bool third_party,
                                const char* resource_type);

/**
 * Adds a tag to the engine for consideration
 */
void engine_add_tag(struct C_Engine* engine, const char* tag);

/**
 * Checks if a tag exists in the engine
 */
bool engine_tag_exists(struct C_Engine* engine, const char* tag);

/**
 * Adds a resource to the engine by name
 */
bool engine_add_resource(struct C_Engine* engine,
                         const char* key,
                         const char* content_type,
                         const char* data);

/**
 * Adds a list of `Resource`s from JSON format
 */
void engine_add_resources(struct C_Engine* engine, const char* resources);

/**
 * Removes a tag to the engine for consideration
 */
void engine_remove_tag(struct C_Engine* engine, const char* tag);

/**
 * Deserializes a previously serialized data file list.
 */
bool engine_deserialize(struct C_Engine* engine,
                        const char* data,
                        size_t data_size);

/**
 * Destroy a `Engine` once you are done with it.
 */
void engine_destroy(struct C_Engine* engine);

/**
 * Puts a pointer to the homepage of the `FilterListMetadata` into `homepage`.
 * Returns `true` if a homepage was returned.
 */
bool filter_list_metadata_homepage(const struct C_FilterListMetadata* metadata,
                                   char** homepage);

/**
 * Puts a pointer to the title of the `FilterListMetadata` into `title`. Returns
 * `true` if a title was returned.
 */
bool filter_list_metadata_title(const struct C_FilterListMetadata* metadata,
                                char** title);

/**
 * Destroy a `FilterListMetadata` once you are done with it.
 */
void filter_list_metadata_destroy(struct C_FilterListMetadata* metadata);

/**
 * Destroy a `*c_char` once you are done with it.
 */
void c_char_buffer_destroy(char* s);

/**
 * Returns a set of cosmetic filtering resources specific to the given url, in
 * JSON format
 */
char* engine_url_cosmetic_resources(struct C_Engine* engine, const char* url);

/**
 * Returns a stylesheet containing all generic cosmetic rules that begin with
 * any of the provided class and id selectors
 *
 * The leading '.' or '#' character should not be provided
 */
char* engine_hidden_class_id_selectors(struct C_Engine* engine,
                                       const char* const* classes,
                                       size_t classes_size,
                                       const char* const* ids,
                                       size_t ids_size,
                                       const char* const* exceptions,
                                       size_t exceptions_size);

#endif /* BRAVE_COMPONENTS_ADBLOCK_RUST_FFI_SRC_LIB_H_ */
