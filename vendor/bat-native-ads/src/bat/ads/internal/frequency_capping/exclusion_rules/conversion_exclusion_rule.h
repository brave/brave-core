/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_FREQUENCY_CAPPING_EXCLUSION_RULES_CONVERSION_EXCLUSION_RULE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_FREQUENCY_CAPPING_EXCLUSION_RULES_CONVERSION_EXCLUSION_RULE_H_

#include <string>

#include "bat/ads/internal/ad_events/ad_event_info_aliases.h"
#include "bat/ads/internal/bundle/creative_ad_info.h"
#include "bat/ads/internal/frequency_capping/exclusion_rules/exclusion_rule_interface.h"

namespace ads {

class ConversionExclusionRule final
    : public ExclusionRuleInterface<CreativeAdInfo> {
 public:
  explicit ConversionExclusionRule(const AdEventList& ad_events);
  ~ConversionExclusionRule() override;

  ConversionExclusionRule(const ConversionExclusionRule&) = delete;
  ConversionExclusionRule& operator=(const ConversionExclusionRule&) = delete;

  std::string GetUuid(const CreativeAdInfo& creative_ad) const override;

  bool ShouldExclude(const CreativeAdInfo& creative_ad) override;

  std::string GetLastMessage() const override;

 private:
  bool ShouldAllow(const CreativeAdInfo& creative_ad);

  bool DoesRespectCap(const AdEventList& ad_events,
                      const CreativeAdInfo& creative_ad);

  bool should_allow_conversion_tracking_ = false;

  AdEventList ad_events_;

  std::string last_message_;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_FREQUENCY_CAPPING_EXCLUSION_RULES_CONVERSION_EXCLUSION_RULE_H_
