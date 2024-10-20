/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_AD_UNITS_CREATIVE_AD_CACHE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_AD_UNITS_CREATIVE_AD_CACHE_H_

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "brave/components/brave_ads/core/internal/tabs/tab_manager_observer.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "third_party/abseil-cpp/absl/types/variant.h"

namespace brave_ads {

using CreativeAdVariant = absl::variant<mojom::CreativeSearchResultAdInfoPtr>;
using CreativeAdVariantMap =
    std::map</*placement_id*/ std::string, CreativeAdVariant>;

using PlacementIdList = std::vector<std::string>;
using PlacementIdMap = std::map</*tab_id*/ int32_t, PlacementIdList>;

class CreativeAdCache final : public TabManagerObserver {
 public:
  CreativeAdCache();

  CreativeAdCache(const CreativeAdCache&) = delete;
  const CreativeAdCache& operator=(const CreativeAdCache&) = delete;

  CreativeAdCache(CreativeAdCache&&) = delete;
  const CreativeAdCache& operator=(CreativeAdCache&&) = delete;

  ~CreativeAdCache() override;

  void MaybeAdd(const std::string& placement_id,
                CreativeAdVariant creative_ad_variant);

  template <typename T>
  std::optional<T> MaybeGet(const std::string& placement_id) {
    std::optional<CreativeAdVariant> creative_ad_variant =
        MaybeGet(placement_id);
    if (!creative_ad_variant) {
      return std::nullopt;
    }

    if (!absl::holds_alternative<T>(*creative_ad_variant)) {
      return std::nullopt;
    }

    return std::move(absl::get<T>(*creative_ad_variant));
  }

 private:
  // TabManagerObserver:
  void OnDidCloseTab(int32_t tab_id) override;

  std::optional<CreativeAdVariant> MaybeGet(const std::string& placement_id);

  void PurgePlacements(int32_t tab_id);

  CreativeAdVariantMap creative_ad_variants_;
  PlacementIdMap placement_ids_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_AD_UNITS_CREATIVE_AD_CACHE_H_
