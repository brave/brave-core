/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_EXCLUSION_RULES_ANTI_TARGETING_EXCLUSION_RULE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_EXCLUSION_RULES_ANTI_TARGETING_EXCLUSION_RULE_H_

#include <string>

#include "base/memory/raw_ref.h"
#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/exclusion_rules/exclusion_rule_alias.h"
#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/exclusion_rules/exclusion_rule_interface.h"

namespace brave_ads {

namespace resource {
class AntiTargeting;
}  // namespace resource

struct CreativeAdInfo;

class AntiTargetingExclusionRule final
    : public ExclusionRuleInterface<CreativeAdInfo> {
 public:
  AntiTargetingExclusionRule(
      const resource::AntiTargeting& anti_targeting_resource,
      BrowsingHistoryList browsing_history);

  AntiTargetingExclusionRule(const AntiTargetingExclusionRule&) = delete;
  AntiTargetingExclusionRule& operator=(const AntiTargetingExclusionRule&) =
      delete;

  AntiTargetingExclusionRule(AntiTargetingExclusionRule&&) noexcept = delete;
  AntiTargetingExclusionRule& operator=(AntiTargetingExclusionRule&&) noexcept =
      delete;

  ~AntiTargetingExclusionRule() override;

  std::string GetUuid(const CreativeAdInfo& creative_ad) const override;

  base::expected<void, std::string> ShouldInclude(
      const CreativeAdInfo& creative_ad) const override;

 private:
  bool DoesRespectCap(const CreativeAdInfo& creative_ad) const;

  const base::raw_ref<const resource::AntiTargeting> anti_targeting_resource_;

  BrowsingHistoryList browsing_history_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_EXCLUSION_RULES_ANTI_TARGETING_EXCLUSION_RULE_H_
