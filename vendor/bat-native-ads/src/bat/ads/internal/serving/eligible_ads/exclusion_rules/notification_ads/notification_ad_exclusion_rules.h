/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_SERVING_ELIGIBLE_ADS_EXCLUSION_RULES_NOTIFICATION_ADS_NOTIFICATION_AD_EXCLUSION_RULES_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_SERVING_ELIGIBLE_ADS_EXCLUSION_RULES_NOTIFICATION_ADS_NOTIFICATION_AD_EXCLUSION_RULES_H_

#include <memory>

#include "bat/ads/internal/serving/eligible_ads/exclusion_rules/exclusion_rules_base.h"

namespace ads {

namespace geographic {
class SubdivisionTargeting;
}  // namespace geographic

namespace resource {
class AntiTargeting;
}  // namespace resource

class DismissedExclusionRule;

namespace notification_ads {

class ExclusionRules final : public ExclusionRulesBase {
 public:
  ExclusionRules(const AdEventList& ad_events,
                 geographic::SubdivisionTargeting* subdivision_targeting,
                 resource::AntiTargeting* anti_targeting_resource,
                 const BrowsingHistoryList& browsing_history);
  ~ExclusionRules() override;

 private:
  ExclusionRules(const ExclusionRules&) = delete;
  ExclusionRules& operator=(const ExclusionRules&) = delete;

  std::unique_ptr<DismissedExclusionRule> dismissed_exclusion_rule_;
};

}  // namespace notification_ads
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_SERVING_ELIGIBLE_ADS_EXCLUSION_RULES_NOTIFICATION_ADS_NOTIFICATION_AD_EXCLUSION_RULES_H_
