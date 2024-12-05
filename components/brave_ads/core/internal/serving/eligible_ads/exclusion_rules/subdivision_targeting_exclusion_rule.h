/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_ELIGIBLE_ADS_EXCLUSION_RULES_SUBDIVISION_TARGETING_EXCLUSION_RULE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_ELIGIBLE_ADS_EXCLUSION_RULES_SUBDIVISION_TARGETING_EXCLUSION_RULE_H_

#include <string>

#include "base/memory/raw_ref.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/exclusion_rule_interface.h"

namespace brave_ads {

class SubdivisionTargeting;
struct CreativeAdInfo;

class SubdivisionTargetingExclusionRule final
    : public ExclusionRuleInterface<CreativeAdInfo> {
 public:
  explicit SubdivisionTargetingExclusionRule(
      const SubdivisionTargeting& subdivision_targeting);

  SubdivisionTargetingExclusionRule(const SubdivisionTargetingExclusionRule&) =
      delete;
  SubdivisionTargetingExclusionRule& operator=(
      const SubdivisionTargetingExclusionRule&) = delete;

  ~SubdivisionTargetingExclusionRule() override;

  std::string GetUuid(const CreativeAdInfo& creative_ad) const override;

  base::expected<void, std::string> ShouldInclude(
      const CreativeAdInfo& creative_ad) const override;

 private:
  bool DoesRespectCap(const CreativeAdInfo& creative_ad) const;

  const raw_ref<const SubdivisionTargeting> subdivision_targeting_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_ELIGIBLE_ADS_EXCLUSION_RULES_SUBDIVISION_TARGETING_EXCLUSION_RULE_H_
