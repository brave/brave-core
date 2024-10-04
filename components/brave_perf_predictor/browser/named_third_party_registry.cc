/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_perf_predictor/browser/named_third_party_registry.h"

#include <optional>
#include <string_view>
#include <tuple>

#include "base/containers/flat_set.h"
#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string_util.h"
#include "base/task/thread_pool.h"
#include "base/values.h"
#include "brave/components/brave_perf_predictor/browser/bandwidth_linreg_parameters.h"
#include "components/grit/brave_components_resources.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "ui/base/resource/resource_bundle.h"
#include "url/gurl.h"

namespace brave_perf_predictor {

namespace {

std::tuple<base::flat_map<std::string, std::string>,
           base::flat_map<std::string, std::string>>
ParseMappings(std::string_view entities, bool discard_irrelevant) {
  base::flat_map<std::string, std::string> entity_by_domain;
  base::flat_map<std::string, std::string> entity_by_root_domain;

  // Parse the JSON
  std::optional<base::Value> document = base::JSONReader::Read(entities);
  if (!document || !document->is_list()) {
    LOG(ERROR) << "Cannot parse the third-party entities list";
    return {};
  }

  // Collect the mappings
  for (auto& item : document->GetList()) {
    const auto& entity = item.GetDict();

    const std::string* entity_name = entity.FindString("name");
    if (!entity_name)
      continue;
    if (discard_irrelevant && !relevant_entity_set.contains(*entity_name)) {
      VLOG(3) << "Irrelevant entity " << *entity_name;
      continue;
    }
    const auto* entity_domains = entity.FindList("domains");
    if (!entity_domains)
      continue;

    for (auto& entity_domain_it : *entity_domains) {
      if (!entity_domain_it.is_string()) {
        continue;
      }
      const std::string_view entity_domain(entity_domain_it.GetString());

      const auto inserted =
          entity_by_domain.emplace(entity_domain, *entity_name);
      if (!inserted.second) {
        VLOG(2) << "Malformed data: duplicate domain " << entity_domain;
      }
      auto root_domain = net::registry_controlled_domains::GetDomainAndRegistry(
          entity_domain,
          net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);

      auto root_entity_entry = entity_by_root_domain.find(root_domain);
      if (root_entity_entry != entity_by_root_domain.end() &&
          root_entity_entry->second != *entity_name) {
        // If there is a clash at root domain level, neither is correct
        entity_by_root_domain.erase(root_entity_entry);
      } else {
        entity_by_root_domain.emplace(root_domain, *entity_name);
      }
    }
  }

  entity_by_domain.shrink_to_fit();
  entity_by_root_domain.shrink_to_fit();
  return std::make_tuple(entity_by_domain, entity_by_root_domain);
}

std::tuple<base::flat_map<std::string, std::string>,
           base::flat_map<std::string, std::string>>
ParseFromResource(int resource_id) {
  // TODO(AndriusA): insert trace event here
  SCOPED_UMA_HISTOGRAM_TIMER(
      "Brave.Savings.NamedThirdPartyRegistry.LoadTimeMS");
  auto& resource_bundle = ui::ResourceBundle::GetSharedInstance();
  std::string data_resource =
      resource_bundle.LoadDataResourceString(resource_id);
  // Parse resource, discarding irrelevant entities
  return ParseMappings(data_resource, true);
}

}  // namespace

bool NamedThirdPartyRegistry::LoadMappings(std::string_view entities,
                                           bool discard_irrelevant) {
  // Reset previous mappings
  entity_by_domain_.clear();
  entity_by_root_domain_.clear();
  initialized_ = false;

  tie(entity_by_domain_, entity_by_root_domain_) =
      ParseMappings(entities, discard_irrelevant);
  if (entity_by_domain_.size() == 0 || entity_by_root_domain_.size() == 0)
    return false;

  initialized_ = true;
  return true;
}

void NamedThirdPartyRegistry::UpdateMappings(
    std::tuple<base::flat_map<std::string, std::string>,
               base::flat_map<std::string, std::string>> entity_mappings) {
  tie(entity_by_domain_, entity_by_root_domain_) = entity_mappings;
  VLOG(2) << "Loaded " << entity_by_domain_.size() << " mappings by domain and "
          << entity_by_root_domain_.size() << " by root domain; size";
  initialized_ = true;
}

std::optional<std::string> NamedThirdPartyRegistry::GetThirdParty(
    std::string_view request_url) const {
  if (!IsInitialized()) {
    VLOG(2) << "Named Third Party Registry not initialized";
    return std::nullopt;
  }

  const GURL url(request_url);
  if (!url.is_valid())
    return std::nullopt;

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

  return std::nullopt;
}

NamedThirdPartyRegistry::NamedThirdPartyRegistry() = default;

NamedThirdPartyRegistry::~NamedThirdPartyRegistry() = default;

void NamedThirdPartyRegistry::InitializeDefault() {
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&ParseFromResource, IDR_THIRD_PARTY_ENTITIES),
      base::BindOnce(&NamedThirdPartyRegistry::UpdateMappings,
                     weak_factory_.GetWeakPtr()));
}

}  // namespace brave_perf_predictor
