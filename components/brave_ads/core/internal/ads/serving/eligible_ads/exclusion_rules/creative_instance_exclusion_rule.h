/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_EXCLUSION_RULES_CREATIVE_INSTANCE_EXCLUSION_RULE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_EXCLUSION_RULES_CREATIVE_INSTANCE_EXCLUSION_RULE_H_

#include <string>

#include "brave/components/brave_ads/core/internal/ads/ad_events/ad_event_info.h"
#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/exclusion_rules/exclusion_rule_interface.h"

namespace brave_ads {

struct CreativeAdInfo;

class CreativeInstanceExclusionRule final
    : public ExclusionRuleInterface<CreativeAdInfo> {
 public:
  explicit CreativeInstanceExclusionRule(AdEventList ad_events);

  CreativeInstanceExclusionRule(const CreativeInstanceExclusionRule&) = delete;
  CreativeInstanceExclusionRule& operator=(
      const CreativeInstanceExclusionRule&) = delete;

  CreativeInstanceExclusionRule(CreativeInstanceExclusionRule&&) noexcept =
      delete;
  CreativeInstanceExclusionRule& operator=(
      CreativeInstanceExclusionRule&&) noexcept = delete;

  ~CreativeInstanceExclusionRule() override;

  std::string GetUuid(const CreativeAdInfo& creative_ad) const override;

  base::expected<void, std::string> ShouldInclude(
      const CreativeAdInfo& creative_ad) const override;

 private:
  AdEventList ad_events_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_EXCLUSION_RULES_CREATIVE_INSTANCE_EXCLUSION_RULE_H_
