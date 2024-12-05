/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_ELIGIBLE_ADS_EXCLUSION_RULES_PER_WEEK_EXCLUSION_RULE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_ELIGIBLE_ADS_EXCLUSION_RULES_PER_WEEK_EXCLUSION_RULE_H_

#include <string>

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/exclusion_rule_interface.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_info.h"

namespace brave_ads {

struct CreativeAdInfo;

class PerWeekExclusionRule final
    : public ExclusionRuleInterface<CreativeAdInfo> {
 public:
  explicit PerWeekExclusionRule(AdEventList ad_events);

  PerWeekExclusionRule(const PerWeekExclusionRule&) = delete;
  PerWeekExclusionRule& operator=(const PerWeekExclusionRule&) = delete;

  ~PerWeekExclusionRule() override;

  std::string GetUuid(const CreativeAdInfo& creative_ad) const override;

  base::expected<void, std::string> ShouldInclude(
      const CreativeAdInfo& creative_ad) const override;

 private:
  AdEventList ad_events_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_ELIGIBLE_ADS_EXCLUSION_RULES_PER_WEEK_EXCLUSION_RULE_H_
