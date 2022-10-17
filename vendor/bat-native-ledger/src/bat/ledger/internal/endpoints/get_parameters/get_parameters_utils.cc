/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>
#include <vector>

#include "bat/ledger/internal/endpoints/get_parameters/get_parameters_utils.h"
#include "bat/ledger/internal/ledger_impl.h"

namespace {

std::vector<std::string> GetList(const std::string& list_name,
                                 const base::Value::Dict& dict) {
  std::vector<std::string> countries;

  if (const auto* list = dict.FindList(list_name)) {
    for (const auto& country : *list) {
      if (country.is_string()) {
        countries.emplace_back(country.GetString());
      }
    }
  }

  return countries;
}

}  // namespace

namespace ledger::endpoints {

absl::optional<base::flat_map<std::string, mojom::RegionsPtr>>
GetWalletProviderRegions(const base::Value::Dict& dict) {
  base::flat_map<std::string, mojom::RegionsPtr> wallet_provider_regions;

  for (const auto [wallet_provider, regions] : dict) {
    const auto* regions_dict = regions.GetIfDict();
    if (!regions_dict) {
      return absl::nullopt;
    }

    wallet_provider_regions.emplace(
        wallet_provider, mojom::Regions::New(GetList("allow", *regions_dict),
                                             GetList("block", *regions_dict)));
  }

  return wallet_provider_regions;
}

}  // namespace ledger::endpoints
