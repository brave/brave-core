/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_EXCLUSION_RULES_DISMISSED_EXCLUSION_RULE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_EXCLUSION_RULES_DISMISSED_EXCLUSION_RULE_H_

#include <string>

#include "brave/components/brave_ads/core/internal/ads/ad_events/ad_event_info.h"
#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/exclusion_rules/exclusion_rule_interface.h"

namespace brave_ads {

struct CreativeAdInfo;

class DismissedExclusionRule final
    : public ExclusionRuleInterface<CreativeAdInfo> {
 public:
  explicit DismissedExclusionRule(AdEventList ad_events);

  DismissedExclusionRule(const DismissedExclusionRule& other) = delete;
  DismissedExclusionRule& operator=(const DismissedExclusionRule& other) =
      delete;

  DismissedExclusionRule(DismissedExclusionRule&& other) noexcept = delete;
  DismissedExclusionRule& operator=(DismissedExclusionRule&& other) noexcept =
      delete;

  ~DismissedExclusionRule() override;

  std::string GetUuid(const CreativeAdInfo& creative_ad) const override;

  bool ShouldExclude(const CreativeAdInfo& creative_ad) override;

  const std::string& GetLastMessage() const override;

 private:
  AdEventList ad_events_;

  std::string last_message_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_EXCLUSION_RULES_DISMISSED_EXCLUSION_RULE_H_
