/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_FREQUENCY_CAPPING_EXCLUSION_RULES_ANTI_TARGETING_FREQUENCY_CAP_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_FREQUENCY_CAPPING_EXCLUSION_RULES_ANTI_TARGETING_FREQUENCY_CAP_H_

#include <string>

#include "bat/ads/ad_info.h"
#include "bat/ads/internal/frequency_capping/exclusion_rules/exclusion_rule.h"
#include "bat/ads/internal/frequency_capping/frequency_capping_aliases.h"

namespace ads {

namespace resource {
class AntiTargeting;
}  // namespace resource

struct CreativeAdInfo;

class AntiTargetingFrequencyCap : public ExclusionRule<CreativeAdInfo> {
 public:
  AntiTargetingFrequencyCap(resource::AntiTargeting* anti_targeting_resource,
                            const BrowsingHistoryList& browsing_history);

  ~AntiTargetingFrequencyCap() override;

  AntiTargetingFrequencyCap(const AntiTargetingFrequencyCap&) = delete;
  AntiTargetingFrequencyCap& operator=(const AntiTargetingFrequencyCap&) =
      delete;

  bool ShouldExclude(const CreativeAdInfo& ad) override;

  std::string get_last_message() const override;

 private:
  resource::AntiTargeting* anti_targeting_resource_;  // NOT OWNED

  BrowsingHistoryList browsing_history_;

  std::string last_message_;

  bool DoesRespectCap(const CreativeAdInfo& ad) const;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_FREQUENCY_CAPPING_EXCLUSION_RULES_ANTI_TARGETING_FREQUENCY_CAP_H_
