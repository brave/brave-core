/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 3.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_COSMETIC_FILTERS_BROWSER_COSMETIC_FILTERS_RESOURCES_H_
#define BRAVE_COMPONENTS_COSMETIC_FILTERS_BROWSER_COSMETIC_FILTERS_RESOURCES_H_

#include <memory>
#include <string>

#include "base/optional.h"
#include "base/values.h"
#include "brave/components/cosmetic_filters/common/cosmetic_filters.mojom.h"

class HostContentSettingsMap;

namespace brave_shields {
class AdBlockService;
}

namespace cosmetic_filters {

class CosmeticFiltersResources final
    : public cosmetic_filters::mojom::CosmeticFiltersResources {
 public:
  CosmeticFiltersResources(const CosmeticFiltersResources&) = delete;
  CosmeticFiltersResources& operator=(const CosmeticFiltersResources&) = delete;
  explicit CosmeticFiltersResources(
      HostContentSettingsMap* settings_map,
      brave_shields::AdBlockService* ad_block_service);
  ~CosmeticFiltersResources() override;

  void ShouldDoCosmeticFiltering(
      const std::string& url,
      ShouldDoCosmeticFilteringCallback callback) override;
  void HiddenClassIdSelectors(const std::string& input,
                              const std::vector<std::string>& exceptions,
                              HiddenClassIdSelectorsCallback callback) override;
  void UrlCosmeticResources(const std::string& url,
                            UrlCosmeticResourcesCallback callback) override;

 private:
  void HiddenClassIdSelectorsOnUI(HiddenClassIdSelectorsCallback callback,
                                  base::Optional<base::Value> resources);

  void UrlCosmeticResourcesOnUI(UrlCosmeticResourcesCallback callback,
                                base::Optional<base::Value> resources);

  HostContentSettingsMap* settings_map_;             // Not owned
  brave_shields::AdBlockService* ad_block_service_;  // Not owned
};

}  // namespace cosmetic_filters

#endif  // BRAVE_COMPONENTS_COSMETIC_FILTERS_BROWSER_COSMETIC_FILTERS_RESOURCES_H_
