/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CREATIVES_AD_NOTIFICATIONS_AD_NOTIFICATION_EXCLUSION_RULES_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CREATIVES_AD_NOTIFICATIONS_AD_NOTIFICATION_EXCLUSION_RULES_H_

#include <memory>

#include "bat/ads/internal/creatives/exclusion_rules_base.h"

namespace ads {

namespace targeting {
namespace geographic {
class SubdivisionTargeting;
}  // namespace geographic
}  // namespace targeting

namespace resource {
class AntiTargeting;
}  // namespace resource

class DismissedExclusionRule;

namespace ad_notifications {
namespace frequency_capping {

class ExclusionRules final : public ExclusionRulesBase {
 public:
  ExclusionRules(
      const AdEventList& ad_events,
      targeting::geographic::SubdivisionTargeting* subdivision_targeting,
      resource::AntiTargeting* anti_targeting_resource,
      const BrowsingHistoryList& browsing_history);
  ~ExclusionRules() override;

 private:
  ExclusionRules(const ExclusionRules&) = delete;
  ExclusionRules& operator=(const ExclusionRules&) = delete;

  std::unique_ptr<DismissedExclusionRule> dismissed_exclusion_rule_;
};

}  // namespace frequency_capping
}  // namespace ad_notifications
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CREATIVES_AD_NOTIFICATIONS_AD_NOTIFICATION_EXCLUSION_RULES_H_
