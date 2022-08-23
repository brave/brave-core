/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/serving/eligible_ads/exclusion_rules/inline_content_ads/inline_content_ad_exclusion_rules.h"

#include "bat/ads/internal/ads/serving/eligible_ads/exclusion_rules/per_hour_exclusion_rule.h"
#include "bat/ads/internal/geographic/subdivision/subdivision_targeting.h"
#include "bat/ads/internal/resources/behavioral/anti_targeting/anti_targeting_resource.h"

namespace ads {
namespace inline_content_ads {

ExclusionRules::ExclusionRules(
    const AdEventList& ad_events,
    geographic::SubdivisionTargeting* subdivision_targeting,
    resource::AntiTargeting* anti_targeting_resource,
    const BrowsingHistoryList& browsing_history)
    : ExclusionRulesBase(ad_events,
                         subdivision_targeting,
                         anti_targeting_resource,
                         browsing_history) {
  per_hour_exclusion_rule_ = std::make_unique<PerHourExclusionRule>(ad_events);
  exclusion_rules_.push_back(per_hour_exclusion_rule_.get());
}

ExclusionRules::~ExclusionRules() = default;

}  // namespace inline_content_ads
}  // namespace ads
