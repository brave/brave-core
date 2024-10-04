/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PERF_PREDICTOR_BROWSER_NAMED_THIRD_PARTY_REGISTRY_H_
#define BRAVE_COMPONENTS_BRAVE_PERF_PREDICTOR_BROWSER_NAMED_THIRD_PARTY_REGISTRY_H_

#include <optional>
#include <string>
#include <string_view>
#include <tuple>

#include "base/containers/flat_map.h"
#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "components/keyed_service/core/keyed_service.h"

namespace brave_perf_predictor {

// Retrieves publicly known Third Party (organisation) for a given URL, using
// data from the Third Party Web repository
// (https://github.com/patrickhulce/third-party-web).
class NamedThirdPartyRegistry : public KeyedService {
 public:
  NamedThirdPartyRegistry();
  ~NamedThirdPartyRegistry() override;

  NamedThirdPartyRegistry(const NamedThirdPartyRegistry&) = delete;
  NamedThirdPartyRegistry& operator=(const NamedThirdPartyRegistry&) = delete;

  // Parse the provided mappings (in JSON format), potentially discarding
  // entities not relevant to the bandwith prediction model (i.e. those not
  // seen in training the model).
  bool LoadMappings(std::string_view entities, bool discard_irrelevant);
  // Default initialization - asynchronously load from bundled resource
  void InitializeDefault();
  std::optional<std::string> GetThirdParty(std::string_view domain) const;

 private:
  bool IsInitialized() const { return initialized_; }
  void MarkInitialized(bool initialized) { initialized_ = initialized; }
  void UpdateMappings(
      std::tuple<base::flat_map<std::string, std::string>,
                 base::flat_map<std::string, std::string>> entity_mappings);

  bool initialized_ = false;
  base::flat_map<std::string, std::string> entity_by_domain_;
  base::flat_map<std::string, std::string> entity_by_root_domain_;

  base::WeakPtrFactory<NamedThirdPartyRegistry> weak_factory_{this};
};

}  // namespace brave_perf_predictor

#endif  // BRAVE_COMPONENTS_BRAVE_PERF_PREDICTOR_BROWSER_NAMED_THIRD_PARTY_REGISTRY_H_
