/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ad_units/creative_ad_cache.h"

#include <optional>

#include "base/containers/contains.h"
#include "base/functional/overloaded.h"
#include "brave/components/brave_ads/core/internal/tabs/tab_info.h"
#include "brave/components/brave_ads/core/internal/tabs/tab_manager.h"

namespace brave_ads {

namespace {

bool IsCreativeAdVariantValid(const CreativeAdVariant& creative_ad_variant) {
  return absl::visit(
      base::Overloaded{
          [](const mojom::CreativeSearchResultAdInfoPtr& search_result_ad)
              -> bool { return !!search_result_ad; }},
      creative_ad_variant);
}

std::optional<CreativeAdVariant> CloneCreativeAdVariant(
    const CreativeAdVariant& creative_ad_variant) {
  return absl::visit(
      base::Overloaded{
          [](const mojom::CreativeSearchResultAdInfoPtr& search_result_ad)
              -> std::optional<CreativeAdVariant> {
            return search_result_ad.Clone();
          }},
      creative_ad_variant);
}

}  // namespace

CreativeAdCache::CreativeAdCache() {
  TabManager::GetInstance().AddObserver(this);
}

CreativeAdCache::~CreativeAdCache() {
  TabManager::GetInstance().RemoveObserver(this);
}

void CreativeAdCache::MaybeAdd(const std::string& placement_id,
                               CreativeAdVariant creative_ad_variant) {
  if (!IsCreativeAdVariantValid(creative_ad_variant)) {
    return;
  }

  if (const std::optional<TabInfo> tab =
          TabManager::GetInstance().MaybeGetVisible()) {
    creative_ad_variants_[placement_id] = std::move(creative_ad_variant);
    placement_ids_[tab->id].push_back(placement_id);
  }
}

std::optional<CreativeAdVariant> CreativeAdCache::MaybeGet(
    const std::string& placement_id) {
  const auto iter = creative_ad_variants_.find(placement_id);
  if (iter == creative_ad_variants_.cend()) {
    return std::nullopt;
  }
  const auto& creative_ad_variant = iter->second;

  return CloneCreativeAdVariant(creative_ad_variant);
}

void CreativeAdCache::OnDidCloseTab(const int32_t tab_id) {
  PurgePlacements(tab_id);
}

void CreativeAdCache::PurgePlacements(const int32_t tab_id) {
  if (!base::Contains(placement_ids_, tab_id)) {
    return;
  }

  for (const std::string& placement_id : placement_ids_[tab_id]) {
    creative_ad_variants_.erase(placement_id);
  }
  placement_ids_.erase(tab_id);
}

}  // namespace brave_ads
