/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_FREQUENCY_CAPPING_EXCLUSION_RULES_SUBDIVISION_TARGETING_FREQUENCY_CAP_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_FREQUENCY_CAPPING_EXCLUSION_RULES_SUBDIVISION_TARGETING_FREQUENCY_CAP_H_

#include <string>

#include "bat/ads/ad_info.h"
#include "bat/ads/internal/bundle/creative_ad_info.h"
#include "bat/ads/internal/frequency_capping/exclusion_rules/exclusion_rule.h"

namespace ads {

namespace ad_targeting {
namespace geographic {
class SubdivisionTargeting;
}  // namespace geographic
}  // namespace ad_targeting

class SubdivisionTargetingFrequencyCap : public ExclusionRule<CreativeAdInfo> {
 public:
  SubdivisionTargetingFrequencyCap(
      ad_targeting::geographic::SubdivisionTargeting* subdivision_targeting);

  ~SubdivisionTargetingFrequencyCap() override;

  SubdivisionTargetingFrequencyCap(const SubdivisionTargetingFrequencyCap&) =
      delete;
  SubdivisionTargetingFrequencyCap& operator=(
      const SubdivisionTargetingFrequencyCap&) = delete;

  bool ShouldExclude(const CreativeAdInfo& ad) override;

  std::string get_last_message() const override;

 private:
  ad_targeting::geographic::SubdivisionTargeting*
      subdivision_targeting_;  // NOT OWNED

  std::string last_message_;

  bool DoesRespectCap(const CreativeAdInfo& ad);
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_FREQUENCY_CAPPING_EXCLUSION_RULES_SUBDIVISION_TARGETING_FREQUENCY_CAP_H_
