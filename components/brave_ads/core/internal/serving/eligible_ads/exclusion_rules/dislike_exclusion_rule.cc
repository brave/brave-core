/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/dislike_exclusion_rule.h"

#include "base/containers/contains.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_info.h"
#include "brave/components/brave_ads/core/internal/deprecated/client/client_state_manager.h"
#include "brave/components/brave_ads/core/internal/deprecated/client/preferences/filtered_advertiser_info.h"

namespace brave_ads {

namespace {

bool DoesRespectCap(const CreativeAdInfo& creative_ad) {
  const FilteredAdvertiserList& filtered_advertisers =
      ClientStateManager::GetInstance().GetFilteredAdvertisers();
  if (filtered_advertisers.empty()) {
    return true;
  }

  return !base::Contains(filtered_advertisers, creative_ad.advertiser_id,
                         &FilteredAdvertiserInfo::id);
}

}  // namespace

std::string DislikeExclusionRule::GetUuid(
    const CreativeAdInfo& creative_ad) const {
  return creative_ad.advertiser_id;
}

base::expected<void, std::string> DislikeExclusionRule::ShouldInclude(
    const CreativeAdInfo& creative_ad) const {
  if (!DoesRespectCap(creative_ad)) {
    return base::unexpected(base::ReplaceStringPlaceholders(
        "advertiserId $1 excluded due to being disliked",
        {creative_ad.advertiser_id}, nullptr));
  }

  return base::ok();
}

}  // namespace brave_ads
