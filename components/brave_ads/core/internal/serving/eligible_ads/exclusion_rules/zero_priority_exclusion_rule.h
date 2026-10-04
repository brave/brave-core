/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_ELIGIBLE_ADS_EXCLUSION_RULES_ZERO_PRIORITY_EXCLUSION_RULE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_ELIGIBLE_ADS_EXCLUSION_RULES_ZERO_PRIORITY_EXCLUSION_RULE_H_

#include <string>

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/exclusion_rule_interface.h"

namespace brave_ads {

struct CreativeAdInfo;

// Excludes creative ads with a priority of 0. Priority-0 ads are never
// servable and must be removed before round-robin so they do not consume
// rotation slots.
class ZeroPriorityExclusionRule final
    : public ExclusionRuleInterface<CreativeAdInfo> {
 public:
  ZeroPriorityExclusionRule();

  ZeroPriorityExclusionRule(const ZeroPriorityExclusionRule&) = delete;
  ZeroPriorityExclusionRule& operator=(const ZeroPriorityExclusionRule&) =
      delete;

  ~ZeroPriorityExclusionRule() override;

  // ExclusionRuleInterface:
  std::string GetCacheKey(const CreativeAdInfo& creative_ad) const override;
  bool ShouldInclude(const CreativeAdInfo& creative_ad) const override;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_ELIGIBLE_ADS_EXCLUSION_RULES_ZERO_PRIORITY_EXCLUSION_RULE_H_
