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
#include "base/memory/weak_ptr.h"
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
  CosmeticFiltersResources(HostContentSettingsMap* settings_map,
                           brave_shields::AdBlockService* ad_block_service);
  ~CosmeticFiltersResources() override;

  // Sends back to renderer a response about rules that has to be applied
  // for the specified selectors.
  void HiddenClassIdSelectors(const std::string& input,
                              const std::vector<std::string>& exceptions,
                              HiddenClassIdSelectorsCallback callback) override;

  // If cosmetic filtering is enabled, sends the renderer a response including
  // whether or not to apply cosmetic filtering to first party elements along
  // with an initial set of rules and scripts to apply for the given URL.
  // If cosmetic filtering is disabled, `enabled` will be set to false and the
  // other return values should be ignored.
  void UrlCosmeticResources(const std::string& url,
                            UrlCosmeticResourcesCallback callback) override;

 private:
  void HiddenClassIdSelectorsOnUI(HiddenClassIdSelectorsCallback callback,
                                  absl::optional<base::Value> resources);

  void UrlCosmeticResourcesOnUI(base::OnceCallback<void(base::Value)> callback,
                                absl::optional<base::Value> resources);

  HostContentSettingsMap* settings_map_;             // Not owned
  brave_shields::AdBlockService* ad_block_service_;  // Not owned

  base::WeakPtrFactory<CosmeticFiltersResources> weak_factory_;
};

}  // namespace cosmetic_filters

#endif  // BRAVE_COMPONENTS_COSMETIC_FILTERS_BROWSER_COSMETIC_FILTERS_RESOURCES_H_
