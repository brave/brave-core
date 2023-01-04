/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/serving/eligible_ads/exclusion_rules/daily_cap_exclusion_rule.h"

#include <utility>

#include "base/strings/stringprintf.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/ads/serving/eligible_ads/exclusion_rules/exclusion_rule_util.h"
#include "bat/ads/internal/creatives/creative_ad_info.h"

namespace ads {

namespace {

bool DoesRespectCap(const AdEventList& ad_events,
                    const CreativeAdInfo& creative_ad) {
  return DoesRespectCampaignCap(creative_ad, ad_events,
                                ConfirmationType::kServed, base::Days(1),
                                creative_ad.daily_cap);
}

}  // namespace

DailyCapExclusionRule::DailyCapExclusionRule(AdEventList ad_events)
    : ad_events_(std::move(ad_events)) {}

DailyCapExclusionRule::~DailyCapExclusionRule() = default;

std::string DailyCapExclusionRule::GetUuid(
    const CreativeAdInfo& creative_ad) const {
  return creative_ad.campaign_id;
}

bool DailyCapExclusionRule::ShouldExclude(const CreativeAdInfo& creative_ad) {
  if (!DoesRespectCap(ad_events_, creative_ad)) {
    last_message_ = base::StringPrintf(
        "campaignId %s has exceeded the dailyCap frequency cap",
        creative_ad.campaign_id.c_str());

    return true;
  }

  return false;
}

const std::string& DailyCapExclusionRule::GetLastMessage() const {
  return last_message_;
}

}  // namespace ads
