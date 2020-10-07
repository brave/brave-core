/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_FREQUENCY_CAPPING_EXCLUSION_RULES_DAILY_CAP_FREQUENCY_CAP_H_  // NOLINT
#define BAT_ADS_INTERNAL_FREQUENCY_CAPPING_EXCLUSION_RULES_DAILY_CAP_FREQUENCY_CAP_H_  // NOLINT

#include <stdint.h>

#include <deque>
#include <map>
#include <string>

#include "bat/ads/internal/bundle/creative_ad_info.h"
#include "bat/ads/internal/frequency_capping/exclusion_rules/exclusion_rule.h"

namespace ads {

class AdsImpl;

class DailyCapFrequencyCap : public ExclusionRule {
 public:
  DailyCapFrequencyCap(
      const AdsImpl* const ads);

  ~DailyCapFrequencyCap() override;

  DailyCapFrequencyCap(const DailyCapFrequencyCap&) = delete;
  DailyCapFrequencyCap& operator=(const DailyCapFrequencyCap&) = delete;

  bool ShouldExclude(
      const CreativeAdInfo& ad) override;

  std::string get_last_message() const override;

 private:
  const AdsImpl* const ads_;  // NOT OWNED

  std::string last_message_;

  bool DoesRespectCap(
      const std::deque<uint64_t>& history,
      const CreativeAdInfo& ad);

  std::deque<uint64_t> FilterHistory(
      const std::map<std::string, std::deque<uint64_t>>& history,
      const std::string& campaign_id);
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_FREQUENCY_CAPPING_EXCLUSION_RULES_DAILY_CAP_FREQUENCY_CAP_H_  // NOLINT
