/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/exclusion_rules/transferred_exclusion_rule.h"

#include <utility>

#include "base/strings/stringprintf.h"
#include "brave/components/brave_ads/core/confirmation_type.h"
#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/exclusion_rules/exclusion_rule_features.h"
#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/exclusion_rules/exclusion_rule_util.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_info.h"

namespace brave_ads {

namespace {

constexpr int kTransferredCap = 1;

bool DoesRespectCap(const AdEventList& ad_events,
                    const CreativeAdInfo& creative_ad) {
  return DoesRespectCampaignCap(
      creative_ad, ad_events, ConfirmationType::kTransferred,
      kShouldExcludeAdIfTransferredWithinTimeWindow.Get(), kTransferredCap);
}

}  // namespace

TransferredExclusionRule::TransferredExclusionRule(AdEventList ad_events)
    : ad_events_(std::move(ad_events)) {}

TransferredExclusionRule::~TransferredExclusionRule() = default;

std::string TransferredExclusionRule::GetUuid(
    const CreativeAdInfo& creative_ad) const {
  return creative_ad.campaign_id;
}

bool TransferredExclusionRule::ShouldExclude(
    const CreativeAdInfo& creative_ad) {
  if (!DoesRespectCap(ad_events_, creative_ad)) {
    last_message_ = base::StringPrintf(
        "campaignId %s has exceeded the transferred frequency cap",
        creative_ad.campaign_id.c_str());

    return true;
  }

  return false;
}

const std::string& TransferredExclusionRule::GetLastMessage() const {
  return last_message_;
}

}  // namespace brave_ads
