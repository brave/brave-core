/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/exclusion_rules/notification_ads/notification_ad_exclusion_rules.h"

#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/exclusion_rules/creative_instance_exclusion_rule.h"
#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/exclusion_rules/notification_ads/notification_ad_dismissed_exclusion_rule.h"
#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/exclusion_rules/notification_ads/notification_ad_embedding_exclusion_rule.h"
#include "brave/components/brave_ads/core/internal/geographic/subdivision/subdivision_targeting.h"
#include "brave/components/brave_ads/core/internal/resources/behavioral/anti_targeting/anti_targeting_resource.h"

namespace brave_ads::notification_ads {

ExclusionRules::ExclusionRules(
    const AdEventList& ad_events,
    geographic::SubdivisionTargeting* subdivision_targeting,
    resource::AntiTargeting* anti_targeting_resource,
    const BrowsingHistoryList& browsing_history)
    : ExclusionRulesBase(ad_events,
                         subdivision_targeting,
                         anti_targeting_resource,
                         browsing_history) {
  creative_instance_exclusion_rule_ =
      std::make_unique<CreativeInstanceExclusionRule>(ad_events);
  exclusion_rules_.push_back(creative_instance_exclusion_rule_.get());

  dismissed_exclusion_rule_ =
      std::make_unique<DismissedExclusionRule>(ad_events);
  exclusion_rules_.push_back(dismissed_exclusion_rule_.get());

  embedding_exclusion_rule_ = std::make_unique<EmbeddingExclusionRule>();
  exclusion_rules_.push_back(embedding_exclusion_rule_.get());
}

ExclusionRules::~ExclusionRules() = default;

}  // namespace brave_ads::notification_ads
