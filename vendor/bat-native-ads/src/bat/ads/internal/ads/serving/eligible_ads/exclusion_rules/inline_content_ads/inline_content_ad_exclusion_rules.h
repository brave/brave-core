/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_EXCLUSION_RULES_INLINE_CONTENT_ADS_INLINE_CONTENT_AD_EXCLUSION_RULES_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_EXCLUSION_RULES_INLINE_CONTENT_ADS_INLINE_CONTENT_AD_EXCLUSION_RULES_H_

#include <memory>

#include "bat/ads/internal/ads/serving/eligible_ads/exclusion_rules/exclusion_rules_base.h"

namespace ads {

namespace geographic {
class SubdivisionTargeting;
}  // namespace geographic

namespace resource {
class AntiTargeting;
}  // namespace resource

class PerHourExclusionRule;

namespace inline_content_ads {

class ExclusionRules final : public ExclusionRulesBase {
 public:
  ExclusionRules(const AdEventList& ad_events,
                 geographic::SubdivisionTargeting* subdivision_targeting,
                 resource::AntiTargeting* anti_targeting_resource,
                 const BrowsingHistoryList& browsing_history);

  ExclusionRules(const ExclusionRules& other) = delete;
  ExclusionRules& operator=(const ExclusionRules& other) = delete;

  ExclusionRules(ExclusionRules&& other) noexcept = delete;
  ExclusionRules& operator=(ExclusionRules&& other) noexcept = delete;

  ~ExclusionRules() override;

 private:
  std::unique_ptr<PerHourExclusionRule> per_hour_exclusion_rule_;
};

}  // namespace inline_content_ads
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_EXCLUSION_RULES_INLINE_CONTENT_ADS_INLINE_CONTENT_AD_EXCLUSION_RULES_H_
