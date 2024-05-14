/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/notification_ads/notification_ad_exclusion_rules.h"

#include <memory>

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/creative_instance_exclusion_rule.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/notification_ads/notification_ad_dismissed_exclusion_rule.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/anti_targeting/resource/anti_targeting_resource.h"
#include "brave/components/brave_ads/core/internal/targeting/geographical/subdivision/subdivision_targeting.h"

namespace brave_ads {

NotificationAdExclusionRules::NotificationAdExclusionRules(
    const AdEventList& ad_events,
    const SubdivisionTargeting& subdivision_targeting,
    const AntiTargetingResource& anti_targeting_resource,
    const BrowsingHistoryList& browsing_history)
    : ExclusionRulesBase(ad_events,
                         subdivision_targeting,
                         anti_targeting_resource,
                         browsing_history) {
  exclusion_rules_.push_back(
      std::make_unique<CreativeInstanceExclusionRule>(ad_events));

  exclusion_rules_.push_back(
      std::make_unique<NotificationAdDismissedExclusionRule>(ad_events));
}

NotificationAdExclusionRules::~NotificationAdExclusionRules() = default;

}  // namespace brave_ads
