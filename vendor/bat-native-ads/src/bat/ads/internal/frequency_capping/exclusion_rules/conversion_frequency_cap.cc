/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/exclusion_rules/conversion_frequency_cap.h"

#include "bat/ads/creative_ad_info.h"
#include "bat/ads/internal/client.h"
#include "bat/ads/internal/frequency_capping/frequency_capping.h"

#include "base/logging.h"
#include "base/strings/stringprintf.h"

namespace ads {

ConversionFrequencyCap::ConversionFrequencyCap(
    const FrequencyCapping* const frequency_capping)
    : frequency_capping_(frequency_capping) {
  DCHECK(frequency_capping_);
}

ConversionFrequencyCap::~ConversionFrequencyCap() = default;

bool ConversionFrequencyCap::ShouldExclude(
    const CreativeAdInfo& ad) {
  if (!DoesRespectCap(ad)) {
    last_message_ = base::StringPrintf("creativeSetId %s has exceeded the "
        "frequency capping for conversions", ad.creative_set_id.c_str());

    return true;
  }

  return false;
}

std::string ConversionFrequencyCap::GetLastMessage() const {
  return last_message_;
}

bool ConversionFrequencyCap::DoesRespectCap(
      const CreativeAdInfo& ad) const {
  auto history =
      frequency_capping_->GetAdConversionHistory(ad.creative_set_id);

  if (history.size() >= 1) {
    return false;
  }

  return true;
}

}  // namespace ads
