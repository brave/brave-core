/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PERF_PREDICTOR_BROWSER_NAMED_THIRD_PARTY_REGISTRY_H_
#define BRAVE_COMPONENTS_BRAVE_PERF_PREDICTOR_BROWSER_NAMED_THIRD_PARTY_REGISTRY_H_

#include <string>

#include "base/containers/flat_map.h"
#include "base/memory/singleton.h"
#include "base/values.h"

namespace brave_perf_predictor {

// Retrieves publicly known Third Party (organisation) for a given URL, using
// data from the Third Party Web repository
// (https://github.com/patrickhulce/third-party-web).
class NamedThirdPartyRegistry {
 public:
  NamedThirdPartyRegistry(const NamedThirdPartyRegistry&) = delete;
  NamedThirdPartyRegistry& operator=(const NamedThirdPartyRegistry&) = delete;
  static NamedThirdPartyRegistry* GetInstance();

  // Parse the provided mappings (in JSON format), potentially discarding
  // entities not relevant to the bandwith prediction model (i.e. those not
  // seen in training the model).
  bool LoadMappings(const base::StringPiece entities,
                    const bool discard_irrelevant);
  base::Optional<std::string> GetThirdParty(
      const base::StringPiece domain) const;

 private:
  friend struct base::DefaultSingletonTraits<NamedThirdPartyRegistry>;

  NamedThirdPartyRegistry();
  ~NamedThirdPartyRegistry();

  bool IsInitialized() const { return initialized_; }
  void MarkInitialized(bool initialized) { initialized_ = initialized; }
  bool InitializeFromResource();

  bool initialized_ = false;
  base::flat_map<std::string, std::string> entity_by_domain_;
  base::flat_map<std::string, std::string> entity_by_root_domain_;
};

}  // namespace brave_perf_predictor

#endif  // BRAVE_COMPONENTS_BRAVE_PERF_PREDICTOR_BROWSER_NAMED_THIRD_PARTY_REGISTRY_H_
