/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_AD_UNITS_CREATIVE_AD_CACHE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_AD_UNITS_CREATIVE_AD_CACHE_H_

#include <cstdint>
#include <map>
#include <optional>
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

  // Adds a creative ad to the cache for the given `placement_id` if it is valid
  // and there is a visible tab.
  void MaybeAdd(const std::string& placement_id,
                CreativeAdVariant creative_ad_variant);

  // Gets a creative ad from the cache if it exists for the given
  // `placement_id`.
  template <typename T>
  std::optional<T> MaybeGet(const std::string& placement_id) {
    std::optional<CreativeAdVariant> creative_ad_variant =
        MaybeGetCreativeAdVariant(placement_id);
    if (!creative_ad_variant) {
      return std::nullopt;
    }

    if (!absl::holds_alternative<T>(*creative_ad_variant)) {
      return std::nullopt;
    }

    return std::move(absl::get<T>(*creative_ad_variant));
  }

 private:
  std::optional<CreativeAdVariant> MaybeGetCreativeAdVariant(
      const std::string& placement_id) const;

  void PurgePlacements(int32_t tab_id);

  // TabManagerObserver:
  void OnDidCloseTab(int32_t tab_id) override;

  CreativeAdVariantMap creative_ad_variants_;
  PlacementIdMap placement_ids_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_AD_UNITS_CREATIVE_AD_CACHE_H_
