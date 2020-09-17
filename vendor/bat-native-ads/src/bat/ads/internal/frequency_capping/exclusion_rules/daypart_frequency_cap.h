/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_FREQUENCY_CAPPING_EXCLUSION_RULES_DAYPART_FREQUENCY_CAP_H_  // NOLINT
#define BAT_ADS_INTERNAL_FREQUENCY_CAPPING_EXCLUSION_RULES_DAYPART_FREQUENCY_CAP_H_  // NOLINT

#include <string>
#include <vector>

#include "bat/ads/internal/bundle/creative_ad_info.h"
#include "bat/ads/internal/frequency_capping/exclusion_rules/exclusion_rule.h"

namespace ads {

class AdsImpl;

class DaypartFrequencyCap : public ExclusionRule {
 public:
  DaypartFrequencyCap(
      const AdsImpl* const ads);

  ~DaypartFrequencyCap() override;

  DaypartFrequencyCap(
      const DaypartFrequencyCap&) = delete;
  DaypartFrequencyCap& operator=(
      const DaypartFrequencyCap&) = delete;

  bool ShouldExclude(
      const CreativeAdInfo& ad) override;

  std::string get_last_message() const override;

  static std::vector<std::string> ParseDaypart(std::string daypart);
 private:
  const AdsImpl* const ads_;  // NOT OWNED

  std::string last_message_;

  bool DoesRespectCap(
      const CreativeAdInfo& ad) const;

  bool HasDayOfWeekMatch(
      const std::string& current_dow,
      const std::string& days_of_week) const;
  bool HasTimeSlotMatch(
      const uint64_t current_minutes_from_start,
      const uint64_t start_time,
      const uint64_t end_time) const;

  std::string GetCurrentDayOfWeek() const;
  uint64_t GetCurrentLocalMinutesFromStart() const;
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_FREQUENCY_CAPPING_EXCLUSION_RULES_SUBDIVISION_TARGETING_FREQUENCY_CAP_H_  // NOLINT
