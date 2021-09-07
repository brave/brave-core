/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_FREQUENCY_CAPPING_EXCLUSION_RULES_TOTAL_MAX_FREQUENCY_CAP_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_FREQUENCY_CAPPING_EXCLUSION_RULES_TOTAL_MAX_FREQUENCY_CAP_H_

#include <string>

#include "bat/ads/internal/ad_events/ad_event_info.h"
#include "bat/ads/internal/bundle/creative_ad_info.h"
#include "bat/ads/internal/frequency_capping/exclusion_rules/exclusion_rule.h"

namespace ads {

class TotalMaxFrequencyCap : public ExclusionRule<CreativeAdInfo> {
 public:
  explicit TotalMaxFrequencyCap(const AdEventList& ad_events);

  ~TotalMaxFrequencyCap() override;

  TotalMaxFrequencyCap(const TotalMaxFrequencyCap&) = delete;
  TotalMaxFrequencyCap& operator=(const TotalMaxFrequencyCap&) = delete;

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

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_FREQUENCY_CAPPING_EXCLUSION_RULES_TOTAL_MAX_FREQUENCY_CAP_H_
