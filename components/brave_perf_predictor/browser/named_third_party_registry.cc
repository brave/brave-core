/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_perf_predictor/browser/named_third_party_registry.h"

#include "base/containers/flat_set.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string_piece.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "brave/components/brave_perf_predictor/browser/bandwidth_linreg_parameters.h"
#include "components/grit/brave_components_resources.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "third_party/re2/src/re2/re2.h"
#include "ui/base/resource/resource_bundle.h"
#include "url/gurl.h"

namespace brave_perf_predictor {

// static
NamedThirdPartyRegistry* NamedThirdPartyRegistry::GetInstance() {
  auto* extractor = base::Singleton<NamedThirdPartyRegistry>::get();
  // By default initialize from packaged resources
  if (!extractor->IsInitialized()) {
    bool initialized = extractor->InitializeFromResource();
    if (!initialized) {
      VLOG(2) << "Initialization from resource failed, marking as initialized "
              << "will not retry";
      extractor->MarkInitialized(true);
    }
  }
  return extractor;
}

bool NamedThirdPartyRegistry::LoadMappings(const base::StringPiece entities,
                                           const bool discard_irrelevant) {
  // Reset previous mappings
  entity_by_domain_.clear();
  entity_by_root_domain_.clear();

  // Parse the JSON
  base::Optional<base::Value> document = base::JSONReader::Read(entities);
  if (!document || !document->is_list()) {
    LOG(ERROR) << "Cannot parse the third-party entities list";
    return false;
  }

  // Collect the mappings
  for (auto& entity : document->GetList()) {
    const std::string* entity_name = entity.FindStringPath("name");
    if (!entity_name)
      continue;
    if (discard_irrelevant && !relevant_entity_set.contains(*entity_name)) {
      VLOG(3) << "Irrelevant entity " << *entity_name;
      continue;
    }
    const auto* entity_domains = entity.FindListPath("domains");
    if (!entity_domains)
      continue;

    for (auto& entity_domain_it : entity_domains->GetList()) {
      if (!entity_domain_it.is_string()) {
        continue;
      }
      const base::StringPiece entity_domain(entity_domain_it.GetString());

      const auto inserted =
          entity_by_domain_.emplace(entity_domain, *entity_name);
      if (!inserted.second) {
        VLOG(2) << "Malformed data: duplicate domain " << entity_domain;
      }
      auto root_domain = net::registry_controlled_domains::GetDomainAndRegistry(
          entity_domain,
          net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);

      auto root_entity_entry = entity_by_root_domain_.find(root_domain);
      if (root_entity_entry != entity_by_root_domain_.end() &&
          root_entity_entry->second != *entity_name) {
        // If there is a clash at root domain level, neither is correct
        entity_by_root_domain_.erase(root_entity_entry);
      } else {
        entity_by_root_domain_.emplace(root_domain, *entity_name);
      }
    }
  }

  entity_by_domain_.shrink_to_fit();
  entity_by_root_domain_.shrink_to_fit();
  VLOG(2) << "Loaded " << entity_by_domain_.size() << " mappings by domain and "
          << entity_by_root_domain_.size() << " by root domain; size";
  initialized_ = true;

  return true;
}

base::Optional<std::string> NamedThirdPartyRegistry::GetThirdParty(
    const base::StringPiece request_url) const {
  const GURL url(request_url);
  if (!url.is_valid())
    return base::nullopt;

  if (url.has_host()) {
    auto domain_entry = entity_by_domain_.find(url.host());
    if (domain_entry != entity_by_domain_.end())
      return domain_entry->second;

    auto root_domain = net::registry_controlled_domains::GetDomainAndRegistry(
        url, net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);

    auto root_domain_entry = entity_by_root_domain_.find(root_domain);
    if (root_domain_entry != entity_by_root_domain_.end())
      return root_domain_entry->second;
  }

  return base::nullopt;
}

NamedThirdPartyRegistry::NamedThirdPartyRegistry() = default;

NamedThirdPartyRegistry::~NamedThirdPartyRegistry() = default;

bool NamedThirdPartyRegistry::InitializeFromResource() {
  const auto resource_id = IDR_THIRD_PARTY_ENTITIES;
  // TODO(AndriusA): insert trace event here
  SCOPED_UMA_HISTOGRAM_TIMER(
      "Brave.Savings.NamedThirdPartyRegistry.LoadTimeMS");
  auto& resource_bundle = ui::ResourceBundle::GetSharedInstance();
  std::string data_resource =
      resource_bundle.LoadDataResourceString(resource_id);
  // Parse resource, discarding irrelevant entities
  return LoadMappings(data_resource, true);
}

}  // namespace brave_perf_predictor
