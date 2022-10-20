/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_EXCLUSION_RULES_PER_WEEK_EXCLUSION_RULE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_EXCLUSION_RULES_PER_WEEK_EXCLUSION_RULE_H_

#include <string>

#include "bat/ads/internal/ads/ad_events/ad_event_info.h"
#include "bat/ads/internal/ads/serving/eligible_ads/exclusion_rules/exclusion_rule_interface.h"

namespace ads {

struct CreativeAdInfo;

class PerWeekExclusionRule final
    : public ExclusionRuleInterface<CreativeAdInfo> {
 public:
  explicit PerWeekExclusionRule(AdEventList ad_events);

  PerWeekExclusionRule(const PerWeekExclusionRule& other) = delete;
  PerWeekExclusionRule& operator=(const PerWeekExclusionRule& other) = delete;

  PerWeekExclusionRule(PerWeekExclusionRule&& other) noexcept = delete;
  PerWeekExclusionRule& operator=(PerWeekExclusionRule&& other) noexcept =
      delete;

  ~PerWeekExclusionRule() override;

  std::string GetUuid(const CreativeAdInfo& creative_ad) const override;

  bool ShouldExclude(const CreativeAdInfo& creative_ad) override;

  const std::string& GetLastMessage() const override;

 private:
  AdEventList ad_events_;

  std::string last_message_;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_EXCLUSION_RULES_PER_WEEK_EXCLUSION_RULE_H_
