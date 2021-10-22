/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/ad_notifications/ad_notification_exclusion_rules.h"

#include "bat/ads/internal/ad_serving/ad_targeting/geographic/subdivision/subdivision_targeting.h"
#include "bat/ads/internal/frequency_capping/exclusion_rules/dismissed_frequency_cap.h"
#include "bat/ads/internal/resources/frequency_capping/anti_targeting_resource.h"

namespace ads {
namespace ad_notifications {
namespace frequency_capping {

ExclusionRules::ExclusionRules(
    const AdEventList& ad_events,
    ad_targeting::geographic::SubdivisionTargeting* subdivision_targeting,
    resource::AntiTargeting* anti_targeting_resource,
    const BrowsingHistoryList& browsing_history)
    : ExclusionRulesBase(ad_events,
                         subdivision_targeting,
                         anti_targeting_resource,
                         browsing_history) {
  dismissed_frequency_cap_ = std::make_unique<DismissedFrequencyCap>(ad_events);
  exclusion_rules_.push_back(dismissed_frequency_cap_.get());
}

ExclusionRules::~ExclusionRules() = default;

}  // namespace frequency_capping
}  // namespace ad_notifications
}  // namespace ads
