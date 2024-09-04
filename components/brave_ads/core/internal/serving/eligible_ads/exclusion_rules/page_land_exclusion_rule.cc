/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/page_land_exclusion_rule.h"

#include <utility>

#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_info.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/exclusion_rule_feature.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/exclusion_rule_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/user_engagement/site_visit/site_visit_feature.h"

namespace brave_ads {

PageLandExclusionRule::PageLandExclusionRule(AdEventList ad_events)
    : ad_events_(std::move(ad_events)) {}

PageLandExclusionRule::~PageLandExclusionRule() = default;

std::string PageLandExclusionRule::GetUuid(
    const CreativeAdInfo& creative_ad) const {
  return creative_ad.campaign_id;
}

base::expected<void, std::string> PageLandExclusionRule::ShouldInclude(
    const CreativeAdInfo& creative_ad) const {
  if (!DoesRespectCampaignCap(
          creative_ad, ad_events_, mojom::ConfirmationType::kLanded,
          kShouldExcludeAdIfLandedOnPageWithinTimeWindow.Get(),
          kPageLandCap.Get())) {
    return base::unexpected(base::ReplaceStringPlaceholders(
        "campaignId $1 has exceeded the page land frequency cap",
        {creative_ad.campaign_id}, nullptr));
  }

  return base::ok();
}

}  // namespace brave_ads
