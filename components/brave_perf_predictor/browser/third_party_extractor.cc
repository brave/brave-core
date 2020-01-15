/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_perf_predictor/browser/third_party_extractor.h"

#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/values.h"
#include "third_party/re2/src/re2/re2.h"

namespace brave_perf_predictor {

RE2 DOMAIN_IN_URL_REGEX(":\\/\\/(.*?)(\\/|$)");
RE2 DOMAIN_CHARACTERS("([a-z0-9.-]+\\.[a-z0-9]+)");
RE2 ROOT_DOMAIN_REGEX("([^.]+\\.([^.]+|(gov|com|co|ne)\\.\\w{2})$)");

base::Optional<std::string> get_domain_from_origin_or_url(
    const std::string& origin_or_url) {
  std::string domain;
  if (RE2::PartialMatch(origin_or_url, DOMAIN_IN_URL_REGEX, &domain))
    return domain;
  if (RE2::PartialMatch(origin_or_url, DOMAIN_CHARACTERS, &domain))
    return domain;
  return base::nullopt;
}

std::string get_root_domain(const std::string& domain) {
  std::string root_domain;
  if (RE2::PartialMatch(domain, ROOT_DOMAIN_REGEX, &root_domain)) {
    return root_domain;
  } else {
    return domain;
  }
}

ThirdPartyExtractor::ThirdPartyExtractor() = default;

bool ThirdPartyExtractor::load_entities(const std::string& entities) {
  // Reset previous mappings
  entity_by_domain_.clear();
  entity_by_root_domain_.clear();

  // Parse the JSON
  base::Optional<base::Value> document = base::JSONReader::Read(entities);
  if (!document || !document->is_list()) {
    LOG(ERROR) << "Bad Document";
    return false;
  }
  base::ListValue* entities_parsed = nullptr;
  if (!document->GetAsList(&entities_parsed))
    return false;

  // Collect the mappings
  for (auto& entity : *entities_parsed) {
    base::DictionaryValue* entity_dict = nullptr;
    if (!entity.GetAsDictionary(&entity_dict))
      continue;
    auto* entity_name = entity_dict->FindKey("name");
    auto* entity_domains = entity_dict->FindKey("domains");
    if (!entity_name || !entity_domains)
      continue;
    std::string entity_name_string;
    if (!entity_name->GetAsString(&entity_name_string))
      continue;
    base::ListValue* entity_domains_list = nullptr;
    if (!entity_domains->GetAsList(&entity_domains_list))
      continue;

    for (auto& entity_domain : *entity_domains_list) {
      std::string entity_domain_string;
      if (!entity_domain.GetAsString(&entity_domain_string))
        continue;

      auto entity_entry = entity_by_domain_.find(entity_domain_string);
      if (entity_entry != entity_by_domain_.end()) {
        LOG(ERROR) << "Duplicate domain " << entity_domain_string;
      } else {
        entity_by_domain_.emplace(entity_domain_string, entity_name_string);
        auto root_domain = get_root_domain(entity_domain_string);

        auto root_entity_entry = entity_by_root_domain_.find(root_domain);
        if (root_entity_entry != entity_by_root_domain_.end() &&
            root_entity_entry->second != entity_name_string) {
          // If there is a clash at root domain level, neither is correct
          entity_by_root_domain_.erase(root_entity_entry);
        } else {
          entity_by_root_domain_.emplace(root_domain, entity_name_string);
        }
      }
    }
  }

  initialized_ = true;

  return true;
}

ThirdPartyExtractor::ThirdPartyExtractor(const std::string& entities) {
  load_entities(entities);
}

ThirdPartyExtractor::~ThirdPartyExtractor() = default;

// static
ThirdPartyExtractor* ThirdPartyExtractor::GetInstance() {
  return base::Singleton<ThirdPartyExtractor>::get();
}

base::Optional<std::string> ThirdPartyExtractor::get_entity(
    const std::string& origin_or_url) {
  base::Optional<std::string> domain =
      get_domain_from_origin_or_url(origin_or_url);
  if (domain.has_value()) {
    auto domain_entry = entity_by_domain_.find(domain.value());
    if (domain_entry != entity_by_domain_.end())
      return domain_entry->second;

    auto root_domain = get_root_domain(domain.value());
    auto root_domain_entry = entity_by_root_domain_.find(root_domain);
    if (root_domain_entry != entity_by_root_domain_.end())
      return root_domain_entry->second;
  }

  return base::nullopt;
}

}  // namespace brave_perf_predictor
