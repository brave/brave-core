/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/command_line_exclusion_rule.h"

#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_info.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state.h"

namespace brave_ads {

namespace {

bool DoesRespectCap(const CreativeAdInfo& creative_ad) {
  const base::flat_map<std::string, bool>& ads_uuids =
      GlobalState::GetInstance()->Flags().ads_uuids;
  if (ads_uuids.empty()) {
    // No command line flags set, respect all ads.
    return true;
  }

  return ads_uuids.contains(creative_ad.creative_instance_id) ||
         ads_uuids.contains(creative_ad.creative_set_id) ||
         ads_uuids.contains(creative_ad.campaign_id) ||
         ads_uuids.contains(creative_ad.advertiser_id);
}

}  // namespace

std::string CommandLineExclusionRule::GetCacheKey(
    const CreativeAdInfo& creative_ad) const {
  // Use creative instance ID as the cache key as it is the most specific ID.
  return creative_ad.creative_instance_id;
}

bool CommandLineExclusionRule::ShouldInclude(
    const CreativeAdInfo& creative_ad) const {
  if (!DoesRespectCap(creative_ad)) {
    BLOG(1, "creativeInstanceId " << creative_ad.creative_instance_id
                                  << " excluded due to being filtered by "
                                     "--ads=uuids= command line arg");
    return false;
  }

  return true;
}

}  // namespace brave_ads
