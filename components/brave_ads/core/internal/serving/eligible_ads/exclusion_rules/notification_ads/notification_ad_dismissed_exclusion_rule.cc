/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/notification_ads/notification_ad_dismissed_exclusion_rule.h"

#include <algorithm>
#include <iterator>
#include <utility>

#include "base/check.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_info.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/exclusion_rule_feature.h"

namespace brave_ads {

namespace {

bool DoesRespectCap(const AdEventList& ad_events) {
  size_t count = 0;

  for (const auto& ad_event : ad_events) {
    if (ad_event.confirmation_type == mojom::ConfirmationType::kClicked) {
      count = 0;
    } else if (ad_event.confirmation_type ==
               mojom::ConfirmationType::kDismissed) {
      ++count;
      if (count >= 2) {
        // An ad was dismissed two or more times in a row without being clicked,
        // so do not show another ad from the same campaign for the specified
        // hours
        return false;
      }
    }
  }

  return true;
}

AdEventList FilterAdEvents(const AdEventList& ad_events,
                           const CreativeAdInfo& creative_ad) {
  const base::TimeDelta time_constraint =
      kShouldExcludeAdIfDismissedWithinTimeWindow.Get();
  if (time_constraint.is_zero()) {
    return {};
  }

  const base::Time now = base::Time::Now();

  AdEventList filtered_ad_events;
  std::ranges::copy_if(
      ad_events, std::back_inserter(filtered_ad_events),
      [now, time_constraint, &creative_ad](const AdEventInfo& ad_event) {
        CHECK(ad_event.created_at);

        return (ad_event.confirmation_type ==
                    mojom::ConfirmationType::kClicked ||
                ad_event.confirmation_type ==
                    mojom::ConfirmationType::kDismissed) &&
               ad_event.type == mojom::AdType::kNotificationAd &&
               ad_event.campaign_id == creative_ad.campaign_id &&
               now - *ad_event.created_at < time_constraint;
      });

  return filtered_ad_events;
}
}  // namespace

NotificationAdDismissedExclusionRule::NotificationAdDismissedExclusionRule(
    AdEventList ad_events)
    : ad_events_(std::move(ad_events)) {}

NotificationAdDismissedExclusionRule::~NotificationAdDismissedExclusionRule() =
    default;

std::string NotificationAdDismissedExclusionRule::GetCacheKey(
    const CreativeAdInfo& creative_ad) const {
  return creative_ad.campaign_id;
}

bool NotificationAdDismissedExclusionRule::ShouldInclude(
    const CreativeAdInfo& creative_ad) const {
  const AdEventList filtered_ad_events =
      FilterAdEvents(ad_events_, creative_ad);
  if (!DoesRespectCap(filtered_ad_events)) {
    BLOG(1, "campaignId " << creative_ad.campaign_id
                          << " has exceeded the dismissed frequency cap");
    return false;
  }

  return true;
}

}  // namespace brave_ads
