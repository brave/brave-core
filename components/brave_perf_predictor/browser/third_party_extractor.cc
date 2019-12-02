/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_perf_predictor/browser/third_party_extractor.h"

#include "base/logging.h"
#include "base/values.h"
#include "third_party/re2/src/re2/re2.h"
#include "rapidjson/document.h"
#include "rapidjson/error/en.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

namespace brave_perf_predictor {

RE2 DOMAIN_IN_URL_REGEX(":\\/\\/(.*?)(\\/|$)");
RE2 DOMAIN_CHARACTERS("([a-z0-9.-]+\\.[a-z0-9]+)");
RE2 ROOT_DOMAIN_REGEX("([^.]+\\.([^.]+|(gov|com|co|ne)\\.\\w{2})$)");

base::Optional<std::string> get_domain_from_origin_or_url(const std::string &origin_or_url) {
  std::string domain;
  if (RE2::PartialMatch(origin_or_url, DOMAIN_IN_URL_REGEX, &domain)) {
    return domain;
  }
  if (RE2::PartialMatch(origin_or_url, DOMAIN_CHARACTERS, &domain)) {
    return domain;
  }
  return base::nullopt;
}

std::string get_root_domain(const std::string &domain) {
  std::string root_domain;
  if (RE2::PartialMatch(domain, ROOT_DOMAIN_REGEX, &root_domain)) {
    return root_domain;
  } else {
    return domain;
  }
}

ThirdPartyExtractor::ThirdPartyExtractor() = default;

bool ThirdPartyExtractor::load_entities(const std::string& entities) {
  rapidjson::Document document;
  document.Parse(entities.c_str());

  if (document.HasParseError()) {
    return false;
  }

  if (!document.IsArray()) {
    return false;
  }

  for (rapidjson::SizeType i = 0; i < document.Size(); i++) {
    const std::string entity_name = document[i]["name"].GetString();
    const rapidjson::Value& entity_domains = document[i]["domains"];
    if (!entity_domains.IsArray()) {
      continue;
    }
    for (rapidjson::SizeType d = 0; d < entity_domains.Size(); d++) {
      auto* entity_domain = entity_domains[d].GetString();
      auto entity_entry = entity_by_domain_.find(entity_domain);
      if (entity_entry != entity_by_domain_.end()) {
        LOG(ERROR) << "Duplicate domain " << entity_domain;
      } else {
        entity_by_domain_.emplace(entity_domain, entity_name);
        auto root_domain = get_root_domain(entity_domain);
        
        auto root_entity_entry = entity_by_root_domain_.find(root_domain);
        if (root_entity_entry != entity_by_root_domain_.end() && root_entity_entry->second != entity_name) {
          // If there is a clash at root domain level, neither mapping is correct
          entity_by_root_domain_.erase(root_entity_entry);
        } else {
          entity_by_root_domain_.emplace(root_domain, entity_name);
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

//static
ThirdPartyExtractor* ThirdPartyExtractor::GetInstance() {
  return base::Singleton<ThirdPartyExtractor>::get();
}


base::Optional<std::string> ThirdPartyExtractor::get_entity(const std::string& origin_or_url) {
  base::Optional<std::string> domain = get_domain_from_origin_or_url(origin_or_url);
  if (domain.has_value()) {
    auto domain_entry = entity_by_domain_.find(domain.value());
    if (domain_entry != entity_by_domain_.end()) {
      return domain_entry->second;
    }

    auto root_domain = get_root_domain(domain.value());
    auto root_domain_entry = entity_by_root_domain_.find(root_domain);
    if (root_domain_entry != entity_by_root_domain_.end()) {
      return root_domain_entry->second;
    }
  }

  return base::nullopt;
}

}