/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 3.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_COSMETIC_FILTERS_BROWSER_COSMETIC_FILTERS_RESOURCES_H_
#define BRAVE_COMPONENTS_COSMETIC_FILTERS_BROWSER_COSMETIC_FILTERS_RESOURCES_H_

#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/values.h"
#include "brave/components/cosmetic_filters/common/cosmetic_filters.mojom.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

class HostContentSettingsMap;

namespace brave_shields {
class AdBlockService;
}

namespace cosmetic_filters {

// CosmeticFiltersResources is a class that is responsible for interaction
// between CosmeticFiltersJSHandler class that lives inside renderer process.

class CosmeticFiltersResources final
    : public cosmetic_filters::mojom::CosmeticFiltersResources {
 public:
  CosmeticFiltersResources(const CosmeticFiltersResources&) = delete;
  CosmeticFiltersResources& operator=(const CosmeticFiltersResources&) = delete;
  explicit CosmeticFiltersResources(
      brave_shields::AdBlockService* ad_block_service);
  ~CosmeticFiltersResources() override;

  // Sends back to renderer a response about rules that has to be applied
  // for the specified selectors.
  void HiddenClassIdSelectors(const std::string& input,
                              const std::vector<std::string>& exceptions,
                              HiddenClassIdSelectorsCallback callback) override;

  // Sends the renderer a response including whether or not to apply cosmetic
  // filtering to first party elements along with an initial set of rules and
  // scripts to apply for the given URL.
  void UrlCosmeticResources(const std::string& url,
                            UrlCosmeticResourcesCallback callback) override;

 private:
  raw_ptr<brave_shields::AdBlockService> ad_block_service_ =
      nullptr;  // Not owned
};

}  // namespace cosmetic_filters

#endif  // BRAVE_COMPONENTS_COSMETIC_FILTERS_BROWSER_COSMETIC_FILTERS_RESOURCES_H_
