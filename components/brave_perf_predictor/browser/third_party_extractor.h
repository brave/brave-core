/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PERF_PREDICTOR_BROWSER_THIRD_PARTY_EXTRACTOR_H_
#define BRAVE_COMPONENTS_BRAVE_PERF_PREDICTOR_BROWSER_THIRD_PARTY_EXTRACTOR_H_

#include <string>

#include "base/containers/flat_map.h"
#include "base/memory/singleton.h"
#include "base/values.h"

namespace brave_perf_predictor {

class ThirdPartyExtractor {
 public:
  static ThirdPartyExtractor* GetInstance();

  bool LoadEntities(const std::string& entities);
  base::Optional<std::string> GetEntity(const std::string& domain) const;

 private:
  friend struct base::DefaultSingletonTraits<ThirdPartyExtractor>;

  ThirdPartyExtractor();
  ~ThirdPartyExtractor();
  ThirdPartyExtractor(const ThirdPartyExtractor&) = delete;
  ThirdPartyExtractor& operator=(const ThirdPartyExtractor&) = delete;

  bool IsInitialized() const { return initialized_; }
  bool InitializeFromResource();

  bool initialized_ = false;
  base::flat_map<std::string, std::string> entity_by_domain_;
  base::flat_map<std::string, std::string> entity_by_root_domain_;
};

}  // namespace brave_perf_predictor

#endif  // BRAVE_COMPONENTS_BRAVE_PERF_PREDICTOR_BROWSER_THIRD_PARTY_EXTRACTOR_H_
