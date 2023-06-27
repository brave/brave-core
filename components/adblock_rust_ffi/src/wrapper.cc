/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/adblock_rust_ffi/src/wrapper.h"

#include <iostream>

extern "C" {
#include "brave/components/adblock_rust_ffi/src/lib.h"
}

namespace adblock {

bool SetDomainResolver(DomainResolverCallback resolver) {
  return set_domain_resolver(resolver);
}

#if BUILDFLAG(IS_IOS)
const std::string ConvertRulesToContentBlockingRules(const std::string& rules,
                                                     bool* truncated) {
  char* content_blocking_json =
      convert_rules_to_content_blocking(rules.c_str(), truncated);
  const std::string result = std::string(content_blocking_json);
  c_char_buffer_destroy(content_blocking_json);
  return result;
}
#endif

FilterListMetadata::FilterListMetadata() = default;

FilterListMetadata::FilterListMetadata(C_FilterListMetadata* metadata) {
  char* str_buffer;

  if (filter_list_metadata_homepage(metadata, &str_buffer)) {
    homepage = absl::make_optional(std::string(str_buffer));
    c_char_buffer_destroy(str_buffer);
  }

  if (filter_list_metadata_title(metadata, &str_buffer)) {
    title = absl::make_optional(std::string(str_buffer));
    c_char_buffer_destroy(str_buffer);
  }

  expires = filter_list_metadata_expires(metadata);
}

FilterListMetadata::FilterListMetadata(const std::string& list)
    : FilterListMetadata(read_list_metadata(list.c_str(), list.size())) {}

FilterListMetadata::FilterListMetadata(const char* data, size_t data_size)
    : FilterListMetadata(read_list_metadata(data, data_size)) {}

FilterListMetadata::~FilterListMetadata() = default;

FilterListMetadata::FilterListMetadata(FilterListMetadata&&) = default;

std::pair<FilterListMetadata, std::unique_ptr<Engine>> engineWithMetadata(
    const std::string& rules) {
  C_FilterListMetadata* c_metadata;
  std::unique_ptr<Engine> engine = std::make_unique<Engine>(
      engine_create_with_metadata(rules.c_str(), &c_metadata));
  FilterListMetadata metadata = FilterListMetadata(c_metadata);
  filter_list_metadata_destroy(c_metadata);

  return std::make_pair(std::move(metadata), std::move(engine));
}

std::pair<FilterListMetadata, std::unique_ptr<Engine>>
engineFromBufferWithMetadata(const char* data, size_t data_size) {
  C_FilterListMetadata* c_metadata;
  std::unique_ptr<Engine> engine = std::make_unique<Engine>(
      engine_create_from_buffer_with_metadata(data, data_size, &c_metadata));
  FilterListMetadata metadata = FilterListMetadata(c_metadata);
  filter_list_metadata_destroy(c_metadata);

  return std::make_pair(std::move(metadata), std::move(engine));
}

AdblockDebugInfo::AdblockDebugInfo() = default;
AdblockDebugInfo::AdblockDebugInfo(const AdblockDebugInfo&) = default;
AdblockDebugInfo::~AdblockDebugInfo() = default;

Engine::Engine(C_Engine* c_engine) : raw(c_engine) {}

Engine::Engine() : raw(engine_create("")) {}

Engine::Engine(const std::string& rules) : raw(engine_create(rules.c_str())) {}

Engine::Engine(const char* data, size_t data_size)
    : raw(engine_create_from_buffer(data, data_size)) {}

void Engine::matches(const std::string& url,
                     const std::string& host,
                     const std::string& tab_host,
                     bool is_third_party,
                     const std::string& resource_type,
                     bool* did_match_rule,
                     bool* did_match_exception,
                     bool* did_match_important,
                     std::string* redirect,
                     std::string* rewritten_url) {
  char* redirect_char_ptr = nullptr;
  char* rewritten_url_ptr = nullptr;
  engine_match(raw, url.c_str(), host.c_str(), tab_host.c_str(), is_third_party,
               resource_type.c_str(), did_match_rule, did_match_exception,
               did_match_important, &redirect_char_ptr, &rewritten_url_ptr);
  if (redirect_char_ptr) {
    if (redirect) {
      *redirect = redirect_char_ptr;
    }
    c_char_buffer_destroy(redirect_char_ptr);
  }
  if (rewritten_url_ptr) {
    if (rewritten_url) {
      *rewritten_url = rewritten_url_ptr;
    }
    c_char_buffer_destroy(rewritten_url_ptr);
  }
}

std::string Engine::getCspDirectives(const std::string& url,
                                     const std::string& host,
                                     const std::string& tab_host,
                                     bool is_third_party,
                                     const std::string& resource_type) {
  char* csp_raw = engine_get_csp_directives(raw, url.c_str(), host.c_str(),
                                            tab_host.c_str(), is_third_party,
                                            resource_type.c_str());
  const std::string csp = std::string(csp_raw);

  c_char_buffer_destroy(csp_raw);
  return csp;
}

bool Engine::deserialize(const char* data, size_t data_size) {
  return engine_deserialize(raw, data, data_size);
}

void Engine::addTag(const std::string& tag) {
  engine_add_tag(raw, tag.c_str());
}

void Engine::removeTag(const std::string& tag) {
  engine_remove_tag(raw, tag.c_str());
}

bool Engine::tagExists(const std::string& tag) {
  return engine_tag_exists(raw, tag.c_str());
}

void Engine::addResource(const std::string& key,
                         const std::string& content_type,
                         const std::string& data) {
  engine_add_resource(raw, key.c_str(), content_type.c_str(), data.c_str());
}

void Engine::useResources(const std::string& resources) {
  engine_use_resources(raw, resources.c_str());
}

const std::string Engine::urlCosmeticResources(const std::string& url) {
  char* resources_raw = engine_url_cosmetic_resources(raw, url.c_str());
  const std::string resources_json = std::string(resources_raw);

  c_char_buffer_destroy(resources_raw);
  return resources_json;
}

const std::string Engine::hiddenClassIdSelectors(
    const std::vector<std::string>& classes,
    const std::vector<std::string>& ids,
    const std::vector<std::string>& exceptions) {
  std::vector<const char*> classes_raw;
  classes_raw.reserve(classes.size());
  for (const auto& classe : classes) {
    classes_raw.push_back(classe.c_str());
  }

  std::vector<const char*> ids_raw;
  ids_raw.reserve(ids.size());
  for (const auto& id : ids) {
    ids_raw.push_back(id.c_str());
  }

  std::vector<const char*> exceptions_raw;
  exceptions_raw.reserve(exceptions.size());
  for (const auto& exception : exceptions) {
    exceptions_raw.push_back(exception.c_str());
  }

  char* stylesheet_raw = engine_hidden_class_id_selectors(
      raw, classes_raw.data(), classes.size(), ids_raw.data(), ids.size(),
      exceptions_raw.data(), exceptions.size());
  const std::string stylesheet = std::string(stylesheet_raw);

  c_char_buffer_destroy(stylesheet_raw);
  return stylesheet;
}

AdblockDebugInfo Engine::getAdblockDebugInfo() {
  AdblockDebugInfo info;
  auto* debug_info_raw = get_engine_debug_info(raw);
  size_t filters_size = 0U;
  engine_debug_info_get_attr(debug_info_raw, &info.compiled_regex_count,
                             &filters_size);
  info.regex_data.reserve(filters_size);
  for (size_t i = 0; i < filters_size; ++i) {
    RegexDebugEntry entry;
    char* regex_raw = nullptr;
    engine_debug_info_get_regex_entry(debug_info_raw, i, &entry.id, &regex_raw,
                                      &entry.unused_sec, &entry.usage_count);
    if (regex_raw) {
      entry.regex = std::string(regex_raw);
      c_char_buffer_destroy(regex_raw);
    }
    info.regex_data.push_back(std::move(entry));
  }
  engine_debug_info_destroy(debug_info_raw);

  return info;
}

void Engine::discardRegex(uint64_t regex_id) {
  discard_regex(raw, regex_id);
}

void Engine::setupDiscardPolicy(const RegexManagerDiscardPolicy& policy) {
  setup_discard_policy(raw, policy.cleanup_interval_sec,
                       policy.discard_unused_sec);
}

Engine::~Engine() {
  engine_destroy(raw);
}

}  // namespace adblock
