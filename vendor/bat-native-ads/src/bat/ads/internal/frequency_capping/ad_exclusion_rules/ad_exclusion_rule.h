/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_FREQUENCY_CAPPING_AD_EXCLUSION_RULES_AD_EXCLUSION_RULE_H_  // NOLINT
#define BAT_ADS_INTERNAL_FREQUENCY_CAPPING_AD_EXCLUSION_RULES_AD_EXCLUSION_RULE_H_  // NOLINT

#include <string>

#include "bat/ads/ad_info.h"

namespace ads {

class AdExclusionRule {
 public:
  virtual ~AdExclusionRule() = default;

  virtual bool ShouldExclude(
      const AdInfo& ad) = 0;

  virtual std::string get_last_message() const = 0;
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_FREQUENCY_CAPPING_AD_EXCLUSION_RULES_AD_EXCLUSION_RULE_H_  // NOLINT
