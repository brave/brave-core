/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_FREQUENCY_CAPPING_EXCLUSION_RULES_CONVERSION_FREQUENCY_CAP_H_  // NOLINT
#define BAT_ADS_INTERNAL_FREQUENCY_CAPPING_EXCLUSION_RULES_CONVERSION_FREQUENCY_CAP_H_  // NOLINT

#include <string>

#include "bat/ads/internal/frequency_capping/exclusion_rule.h"

namespace ads {

class FrequencyCapping;
struct CreativeAdnfo;

class ConversionFrequencyCap : public ExclusionRule {
 public:
  ConversionFrequencyCap(
      const FrequencyCapping* const frequency_capping);

  ~ConversionFrequencyCap() override;

  ConversionFrequencyCap(const ConversionFrequencyCap&) = delete;
  ConversionFrequencyCap& operator=(const ConversionFrequencyCap&) = delete;

  bool ShouldExclude(
      const CreativeAdInfo& ad) override;

  std::string GetLastMessage() const override;

 private:
  const FrequencyCapping* const frequency_capping_;  // NOT OWNED

  std::string last_message_;

  bool DoesRespectCap(
      const CreativeAdInfo& ad) const;
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_FREQUENCY_CAPPING_EXCLUSION_RULES_CONVERSION_FREQUENCY_CAP_H_  // NOLINT
