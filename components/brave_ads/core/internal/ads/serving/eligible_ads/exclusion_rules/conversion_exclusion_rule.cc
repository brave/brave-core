/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/exclusion_rules/conversion_exclusion_rule.h"

#include <utility>

#include "base/ranges/algorithm.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/exclusion_rules/exclusion_rule_features.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_info.h"

namespace brave_ads {

namespace {

constexpr int kConversionCap = 1;

bool DoesRespectCap(const AdEventList& ad_events,
                    const CreativeAdInfo& creative_ad) {
  const int count = base::ranges::count_if(
      ad_events, [&creative_ad](const AdEventInfo& ad_event) {
        return ad_event.confirmation_type == ConfirmationType::kConversion &&
               ad_event.creative_set_id == creative_ad.creative_set_id;
      });

  return count < kConversionCap;
}

}  // namespace

ConversionExclusionRule::ConversionExclusionRule(AdEventList ad_events)
    : ad_events_(std::move(ad_events)) {}

ConversionExclusionRule::~ConversionExclusionRule() = default;

std::string ConversionExclusionRule::GetUuid(
    const CreativeAdInfo& creative_ad) const {
  return creative_ad.creative_set_id;
}

bool ConversionExclusionRule::ShouldExclude(const CreativeAdInfo& creative_ad) {
  if (!kShouldExcludeAdIfConverted.Get()) {
    return false;
  }

  if (!DoesRespectCap(ad_events_, creative_ad)) {
    last_message_ = base::StringPrintf(
        "creativeSetId %s has exceeded the conversions frequency cap",
        creative_ad.creative_set_id.c_str());

    return true;
  }

  return false;
}

const std::string& ConversionExclusionRule::GetLastMessage() const {
  return last_message_;
}

}  // namespace brave_ads
