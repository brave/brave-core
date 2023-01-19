/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_EXCLUSION_RULES_ANTI_TARGETING_EXCLUSION_RULE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_EXCLUSION_RULES_ANTI_TARGETING_EXCLUSION_RULE_H_

#include <string>

#include "bat/ads/internal/ads/serving/eligible_ads/exclusion_rules/exclusion_rule_alias.h"
#include "bat/ads/internal/ads/serving/eligible_ads/exclusion_rules/exclusion_rule_interface.h"
#include "bat/ads/internal/resources/behavioral/anti_targeting/anti_targeting_info.h"

namespace ads {

namespace resource {
class AntiTargeting;
}  // namespace resource

struct CreativeAdInfo;

class AntiTargetingExclusionRule final
    : public ExclusionRuleInterface<CreativeAdInfo> {
 public:
  AntiTargetingExclusionRule(resource::AntiTargeting* anti_targeting_resource,
                             BrowsingHistoryList browsing_history);

  AntiTargetingExclusionRule(const AntiTargetingExclusionRule& other) = delete;
  AntiTargetingExclusionRule& operator=(const AntiTargetingExclusionRule&) =
      delete;

  AntiTargetingExclusionRule(AntiTargetingExclusionRule&& other) noexcept =
      delete;
  AntiTargetingExclusionRule& operator=(
      AntiTargetingExclusionRule&& other) noexcept = delete;

  ~AntiTargetingExclusionRule() override;

  std::string GetUuid(const CreativeAdInfo& creative_ad) const override;

  bool ShouldExclude(const CreativeAdInfo& creative_ad) override;

  const std::string& GetLastMessage() const override;

 private:
  bool DoesRespectCap(const CreativeAdInfo& creative_ad) const;

  resource::AntiTargetingInfo anti_targeting_;

  BrowsingHistoryList browsing_history_;

  std::string last_message_;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_EXCLUSION_RULES_ANTI_TARGETING_EXCLUSION_RULE_H_
