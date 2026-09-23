/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/zero_priority_exclusion_rule.h"

#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_info.h"

namespace brave_ads {

ZeroPriorityExclusionRule::ZeroPriorityExclusionRule() = default;

ZeroPriorityExclusionRule::~ZeroPriorityExclusionRule() = default;

std::string ZeroPriorityExclusionRule::GetCacheKey(
    const CreativeAdInfo& creative_ad) const {
  return creative_ad.creative_instance_id;
}

bool ZeroPriorityExclusionRule::ShouldInclude(
    const CreativeAdInfo& creative_ad) const {
  if (creative_ad.priority == 0) {
    BLOG(1, "creativeInstanceId " << creative_ad.creative_instance_id
                                  << " excluded because priority is 0");
    return false;
  }

  return true;
}

}  // namespace brave_ads
