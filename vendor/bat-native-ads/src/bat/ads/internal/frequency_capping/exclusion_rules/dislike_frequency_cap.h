/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_FREQUENCY_CAPPING_EXCLUSION_RULES_DISLIKE_FREQUENCY_CAP_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_FREQUENCY_CAPPING_EXCLUSION_RULES_DISLIKE_FREQUENCY_CAP_H_

#include <string>

#include "bat/ads/internal/frequency_capping/exclusion_rules/exclusion_rule.h"

namespace ads {

struct CreativeAdInfo;

class DislikeFrequencyCap : public ExclusionRule<CreativeAdInfo> {
 public:
  DislikeFrequencyCap();

  ~DislikeFrequencyCap() override;

  DislikeFrequencyCap(const DislikeFrequencyCap&) = delete;
  DislikeFrequencyCap& operator=(const DislikeFrequencyCap&) = delete;

  bool ShouldExclude(const CreativeAdInfo& ad) override;

  std::string get_last_message() const override;

 private:
  std::string last_message_;

  bool DoesRespectCap(const CreativeAdInfo& ad);
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_FREQUENCY_CAPPING_EXCLUSION_RULES_DISLIKE_FREQUENCY_CAP_H_
