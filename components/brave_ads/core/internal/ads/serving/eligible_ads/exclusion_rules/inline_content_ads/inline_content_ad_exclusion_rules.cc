/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/exclusion_rules/inline_content_ads/inline_content_ad_exclusion_rules.h"

#include <memory>
#include <utility>

#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/exclusion_rules/creative_instance_exclusion_rule.h"
#include "brave/components/brave_ads/core/internal/geographic/subdivision/subdivision_targeting.h"
#include "brave/components/brave_ads/core/internal/resources/behavioral/anti_targeting/anti_targeting_resource.h"

namespace brave_ads {

InlineContentAdExclusionRules::InlineContentAdExclusionRules(
    const AdEventList& ad_events,
    const SubdivisionTargeting& subdivision_targeting,
    const AntiTargetingResource& anti_targeting_resource,
    const BrowsingHistoryList& browsing_history)
    : ExclusionRulesBase(ad_events,
                         subdivision_targeting,
                         anti_targeting_resource,
                         browsing_history) {
  auto creative_instance_exclusion_rule =
      std::make_unique<CreativeInstanceExclusionRule>(ad_events);
  exclusion_rules_.push_back(std::move(creative_instance_exclusion_rule));
}

InlineContentAdExclusionRules::~InlineContentAdExclusionRules() = default;

}  // namespace brave_ads
