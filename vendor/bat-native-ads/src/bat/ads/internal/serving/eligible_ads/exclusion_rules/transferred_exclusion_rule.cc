/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/serving/eligible_ads/exclusion_rules/transferred_exclusion_rule.h"

#include "base/strings/stringprintf.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/serving/eligible_ads/exclusion_rules/exclusion_rule_features.h"
#include "bat/ads/internal/serving/eligible_ads/exclusion_rules/exclusion_rule_util.h"

namespace ads {

namespace {
constexpr int kTransferredCap = 1;
}  // namespace

TransferredExclusionRule::TransferredExclusionRule(const AdEventList& ad_events)
    : ad_events_(ad_events) {}

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

std::string TransferredExclusionRule::GetLastMessage() const {
  return last_message_;
}

bool TransferredExclusionRule::DoesRespectCap(
    const AdEventList& ad_events,
    const CreativeAdInfo& creative_ad) {
  const base::TimeDelta time_constraint =
      exclusion_rules::features::ExcludeAdIfTransferredWithinTimeWindow();

  return DoesRespectCampaignCap(creative_ad, ad_events,
                                ConfirmationType::kTransferred, time_constraint,
                                kTransferredCap);
}

}  // namespace ads
