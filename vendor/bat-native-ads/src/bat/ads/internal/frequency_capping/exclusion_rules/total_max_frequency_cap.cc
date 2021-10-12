/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/exclusion_rules/total_max_frequency_cap.h"

#include <algorithm>
#include <iterator>

#include "base/strings/stringprintf.h"
#include "bat/ads/internal/frequency_capping/frequency_capping_util.h"

namespace ads {

TotalMaxFrequencyCap::TotalMaxFrequencyCap(const AdEventList& ad_events)
    : ad_events_(ad_events) {}

TotalMaxFrequencyCap::~TotalMaxFrequencyCap() = default;

std::string TotalMaxFrequencyCap::GetUuid(
    const CreativeAdInfo& creative_ad) const {
  return creative_ad.creative_set_id;
}

bool TotalMaxFrequencyCap::ShouldExclude(const CreativeAdInfo& creative_ad) {
  if (!DoesRespectCap(ad_events_, creative_ad)) {
    last_message_ = base::StringPrintf(
        "creativeSetId %s has exceeded the totalMax frequency cap",
        creative_ad.creative_set_id.c_str());

    return true;
  }

  return false;
}

std::string TotalMaxFrequencyCap::GetLastMessage() const {
  return last_message_;
}

bool TotalMaxFrequencyCap::DoesRespectCap(const AdEventList& ad_events,
                                          const CreativeAdInfo& creative_ad) {
  const int count = std::count_if(
      ad_events.cbegin(), ad_events.cend(),
      [&creative_ad](const AdEventInfo& ad_event) {
        return ad_event.confirmation_type == ConfirmationType::kServed &&
               ad_event.creative_set_id == creative_ad.creative_set_id &&
               DoesAdTypeSupportFrequencyCapping(ad_event.type);
      });

  if (count >= creative_ad.total_max) {
    return false;
  }

  return true;
}

}  // namespace ads
