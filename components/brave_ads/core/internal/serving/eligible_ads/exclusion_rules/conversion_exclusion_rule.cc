/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/conversion_exclusion_rule.h"

#include <cstddef>
#include <utility>

#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/internal/common/ranges/algorithm.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_info.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/exclusion_rule_feature.h"

namespace brave_ads {

namespace {

constexpr size_t kConversionCap = 1;

bool DoesRespectCap(const AdEventList& ad_events,
                    const CreativeAdInfo& creative_ad) {
  if (!kShouldExcludeAdIfConverted.Get()) {
    return true;
  }

  const size_t count = count_if_until(
      ad_events,
      [&creative_ad](const AdEventInfo& ad_event) {
        return ad_event.confirmation_type == ConfirmationType::kConversion &&
               ad_event.creative_set_id == creative_ad.creative_set_id;
      },
      kConversionCap);

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

base::expected<void, std::string> ConversionExclusionRule::ShouldInclude(
    const CreativeAdInfo& creative_ad) const {
  if (!DoesRespectCap(ad_events_, creative_ad)) {
    return base::unexpected(base::ReplaceStringPlaceholders(
        "creativeSetId $1 has exceeded the conversions frequency cap",
        {creative_ad.creative_set_id}, nullptr));
  }

  return base::ok();
}

}  // namespace brave_ads
