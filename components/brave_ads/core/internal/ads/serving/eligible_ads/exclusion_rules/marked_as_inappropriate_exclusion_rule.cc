/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/exclusion_rules/marked_as_inappropriate_exclusion_rule.h"

#include "base/containers/contains.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_info.h"
#include "brave/components/brave_ads/core/internal/deprecated/client/client_state_manager.h"
#include "brave/components/brave_ads/core/internal/deprecated/client/preferences/flagged_ad_info.h"

namespace brave_ads {

namespace {

bool DoesRespectCap(const CreativeAdInfo& creative_ad) {
  const FlaggedAdList& flagged_ads =
      ClientStateManager::GetInstance().GetFlaggedAds();
  if (flagged_ads.empty()) {
    return true;
  }

  return !base::Contains(flagged_ads, creative_ad.creative_set_id,
                         &FlaggedAdInfo::creative_set_id);
}

}  // namespace

std::string MarkedAsInappropriateExclusionRule::GetUuid(
    const CreativeAdInfo& creative_ad) const {
  return creative_ad.creative_set_id;
}

base::expected<void, std::string>
MarkedAsInappropriateExclusionRule::ShouldInclude(
    const CreativeAdInfo& creative_ad) const {
  if (!DoesRespectCap(creative_ad)) {
    return base::unexpected(base::ReplaceStringPlaceholders(
        "creativeSetId $1 excluded due to being marked as inappropriate",
        {creative_ad.creative_set_id}, nullptr));
  }

  return base::ok();
}

}  // namespace brave_ads
