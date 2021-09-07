/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_FREQUENCY_CAPPING_EXCLUSION_RULES_PER_MONTH_FREQUENCY_CAP_H_  // NOLINT
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_FREQUENCY_CAPPING_EXCLUSION_RULES_PER_MONTH_FREQUENCY_CAP_H_  // NOLINT

#include <string>

#include "bat/ads/internal/ad_events/ad_event_info.h"
#include "bat/ads/internal/bundle/creative_ad_info.h"
#include "bat/ads/internal/frequency_capping/exclusion_rules/exclusion_rule.h"

namespace ads {

class PerMonthFrequencyCap : public ExclusionRule<CreativeAdInfo> {
 public:
  explicit PerMonthFrequencyCap(const AdEventList& ad_events);

  ~PerMonthFrequencyCap() override;

  PerMonthFrequencyCap(const PerMonthFrequencyCap&) = delete;
  PerMonthFrequencyCap& operator=(const PerMonthFrequencyCap&) = delete;

  bool ShouldExclude(const CreativeAdInfo& ad) override;

  std::string get_last_message() const override;

 private:
  AdEventList ad_events_;

  std::string last_message_;

  bool DoesRespectCap(const AdEventList& ad_events, const CreativeAdInfo& ad);

  AdEventList FilterAdEvents(const AdEventList& ad_events,
                             const CreativeAdInfo& ad) const;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_FREQUENCY_CAPPING_EXCLUSION_RULES_PER_MONTH_FREQUENCY_CAP_H_
