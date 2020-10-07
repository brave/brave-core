/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_FREQUENCY_CAPPING_EXCLUSION_RULES_DISMISSED_CAP_FREQUENCY_CAP_H_  // NOLINT
#define BAT_ADS_INTERNAL_FREQUENCY_CAPPING_EXCLUSION_RULES_DISMISSED_CAP_FREQUENCY_CAP_H_  // NOLINT

#include <deque>
#include <string>

#include "bat/ads/ad_history.h"
#include "bat/ads/internal/bundle/creative_ad_info.h"
#include "bat/ads/internal/frequency_capping/exclusion_rules/exclusion_rule.h"

namespace ads {

class AdsImpl;

class DismissedFrequencyCap : public ExclusionRule {
 public:
  DismissedFrequencyCap(
      const AdsImpl* const ads);

  ~DismissedFrequencyCap() override;

  DismissedFrequencyCap(const DismissedFrequencyCap&) = delete;
  DismissedFrequencyCap& operator=(const DismissedFrequencyCap&) = delete;

  bool ShouldExclude(
      const CreativeAdInfo& ad) override;

  std::string get_last_message() const override;

 private:
  const AdsImpl* const ads_;  // NOT OWNED

  std::string last_message_;

  bool DoesRespectCap(
      const std::deque<AdHistory>& history,
      const CreativeAdInfo& ad);

  std::deque<AdHistory> FilterHistory(
      const std::deque<AdHistory>& history,
      const std::string& campaign_id);
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_FREQUENCY_CAPPING_EXCLUSION_RULES_DISMISSED_CAP_FREQUENCY_CAP_H_  // NOLINT
