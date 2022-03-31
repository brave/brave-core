/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "wrapper.h"  // NOLINT https://github.com/brave/brave-browser/issues/14821
#include <iostream>

extern "C" {
#include "lib.h"  // NOLINT
}

namespace adblock {

bool SetDomainResolver(DomainResolverCallback resolver) {
  return set_domain_resolver(resolver);
}

std::vector<FilterList> FilterList::default_list;
std::vector<FilterList> FilterList::regional_list;

FilterList::FilterList(const std::string& uuid,
                       const std::string& url,
                       const std::string& title,
                       const std::vector<std::string>& langs,
                       const std::string& support_url,
                       const std::string& component_id,
                       const std::string& base64_public_key,
                       const std::string& desc)
    : uuid(uuid),
      url(url),
      title(title),
      langs(langs),
      support_url(support_url),
      component_id(component_id),
      base64_public_key(base64_public_key),
      desc(desc) {}

FilterList::FilterList(const FilterList& other) = default;

FilterList::~FilterList() {}

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
                     std::string* redirect) {
  char* redirect_char_ptr = nullptr;
  engine_match(raw, url.c_str(), host.c_str(), tab_host.c_str(), is_third_party,
               resource_type.c_str(), did_match_rule, did_match_exception,
               did_match_important, &redirect_char_ptr);
  if (redirect_char_ptr) {
    if (redirect) {
      *redirect = redirect_char_ptr;
    }
    c_char_buffer_destroy(redirect_char_ptr);
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

void Engine::addResources(const std::string& resources) {
  engine_add_resources(raw, resources.c_str());
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
  for (size_t i = 0; i < classes.size(); i++) {
    classes_raw.push_back(classes[i].c_str());
  }

  std::vector<const char*> ids_raw;
  ids_raw.reserve(ids.size());
  for (size_t i = 0; i < ids.size(); i++) {
    ids_raw.push_back(ids[i].c_str());
  }

  std::vector<const char*> exceptions_raw;
  exceptions_raw.reserve(exceptions.size());
  for (size_t i = 0; i < exceptions.size(); i++) {
    exceptions_raw.push_back(exceptions[i].c_str());
  }

  char* stylesheet_raw = engine_hidden_class_id_selectors(
      raw, classes_raw.data(), classes.size(), ids_raw.data(), ids.size(),
      exceptions_raw.data(), exceptions.size());
  const std::string stylesheet = std::string(stylesheet_raw);

  c_char_buffer_destroy(stylesheet_raw);
  return stylesheet;
}

Engine::~Engine() {
  engine_destroy(raw);
}

}  // namespace adblock
