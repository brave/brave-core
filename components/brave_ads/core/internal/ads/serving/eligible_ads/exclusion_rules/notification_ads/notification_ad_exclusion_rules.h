/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_EXCLUSION_RULES_NOTIFICATION_ADS_NOTIFICATION_AD_EXCLUSION_RULES_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_EXCLUSION_RULES_NOTIFICATION_ADS_NOTIFICATION_AD_EXCLUSION_RULES_H_

#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/exclusion_rules/exclusion_rules_base.h"

namespace brave_ads {

class AntiTargetingResource;
class SubdivisionTargeting;

class NotificationAdExclusionRules final : public ExclusionRulesBase {
 public:
  NotificationAdExclusionRules(
      const AdEventList& ad_events,
      const SubdivisionTargeting& subdivision_targeting,
      const AntiTargetingResource& anti_targeting_resource,
      const BrowsingHistoryList& browsing_history);

  NotificationAdExclusionRules(const NotificationAdExclusionRules&) = delete;
  NotificationAdExclusionRules& operator=(const NotificationAdExclusionRules&) =
      delete;

  NotificationAdExclusionRules(NotificationAdExclusionRules&&) noexcept =
      delete;
  NotificationAdExclusionRules& operator=(
      NotificationAdExclusionRules&&) noexcept = delete;

  ~NotificationAdExclusionRules() override;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_EXCLUSION_RULES_NOTIFICATION_ADS_NOTIFICATION_AD_EXCLUSION_RULES_H_
