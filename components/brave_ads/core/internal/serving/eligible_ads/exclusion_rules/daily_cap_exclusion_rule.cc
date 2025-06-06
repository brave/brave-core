/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/daily_cap_exclusion_rule.h"

#include <utility>

#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_info.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/exclusion_rule_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads {

DailyCapExclusionRule::DailyCapExclusionRule(AdEventList ad_events)
    : ad_events_(std::move(ad_events)) {}

DailyCapExclusionRule::~DailyCapExclusionRule() = default;

std::string DailyCapExclusionRule::GetCacheKey(
    const CreativeAdInfo& creative_ad) const {
  return creative_ad.campaign_id;
}

bool DailyCapExclusionRule::ShouldInclude(
    const CreativeAdInfo& creative_ad) const {
  if (!DoesRespectCampaignCap(creative_ad, ad_events_,
                              mojom::ConfirmationType::kServedImpression,
                              base::Days(1), creative_ad.daily_cap)) {
    BLOG(1, "campaignId " << creative_ad.campaign_id
                          << " has exceeded the dailyCap frequency cap");
    return false;
  }

  return true;
}

}  // namespace brave_ads
