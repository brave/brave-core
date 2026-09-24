/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/total_max_exclusion_rule.h"

#include <utility>

#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_info.h"

namespace brave_ads {

namespace {

bool DoesRespectCap(const AdEventList& ad_events,
                    const CreativeAdInfo& creative_ad) {
  if (creative_ad.total_max == 0) {
    // Always respect cap if set to 0.
    return true;
  }

  int count = 0;
  for (const auto& ad_event : ad_events) {
    if (ad_event.confirmation_type ==
            mojom::ConfirmationType::kServedImpression &&
        ad_event.creative_set_id == creative_ad.creative_set_id) {
      ++count;
      if (count >= creative_ad.total_max) {
        return false;
      }
    }
  }

  return true;
}

}  // namespace

TotalMaxExclusionRule::TotalMaxExclusionRule(AdEventList ad_events)
    : ad_events_(std::move(ad_events)) {}

TotalMaxExclusionRule::~TotalMaxExclusionRule() = default;

std::string TotalMaxExclusionRule::GetCacheKey(
    const CreativeAdInfo& creative_ad) const {
  return creative_ad.creative_set_id;
}

bool TotalMaxExclusionRule::ShouldInclude(
    const CreativeAdInfo& creative_ad) const {
  if (!DoesRespectCap(ad_events_, creative_ad)) {
    BLOG(1, "creativeSetId " << creative_ad.creative_set_id
                             << " has exceeded the totalMax frequency cap");
    return false;
  }

  return true;
}

}  // namespace brave_ads
