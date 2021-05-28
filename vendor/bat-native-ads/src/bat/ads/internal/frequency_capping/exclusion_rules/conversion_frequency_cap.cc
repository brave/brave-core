/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/exclusion_rules/conversion_frequency_cap.h"

#include <cstdint>

#include "base/strings/stringprintf.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/bundle/creative_ad_info.h"
#include "bat/ads/pref_names.h"

namespace ads {

namespace {
const uint64_t kConversionFrequencyCap = 1;
}  // namespace

ConversionFrequencyCap::ConversionFrequencyCap(const AdEventList& ad_events)
    : ad_events_(ad_events) {}

ConversionFrequencyCap::~ConversionFrequencyCap() = default;

bool ConversionFrequencyCap::ShouldExclude(const CreativeAdInfo& ad) {
  if (!ShouldAllow(ad)) {
    last_message_ = base::StringPrintf(
        "creativeSetId %s excluded due to ad conversion tracking being "
        "disabled",
        ad.creative_set_id.c_str());

    return true;
  }

  const AdEventList filtered_ad_events = FilterAdEvents(ad_events_, ad);

  if (!DoesRespectCap(filtered_ad_events)) {
    last_message_ = base::StringPrintf(
        "creativeSetId %s has exceeded the frequency capping for conversions",
        ad.creative_set_id.c_str());

    return true;
  }

  return false;
}

std::string ConversionFrequencyCap::get_last_message() const {
  return last_message_;
}

bool ConversionFrequencyCap::ShouldAllow(const CreativeAdInfo& ad) {
  if (ad.conversion && !AdsClientHelper::Get()->GetBooleanPref(
                           prefs::kShouldAllowConversionTracking)) {
    return false;
  }

  return true;
}

bool ConversionFrequencyCap::DoesRespectCap(const AdEventList& ad_events) {
  if (ad_events.size() >= kConversionFrequencyCap) {
    return false;
  }

  return true;
}

AdEventList ConversionFrequencyCap::FilterAdEvents(
    const AdEventList& ad_events,
    const CreativeAdInfo& ad) const {
  AdEventList filtered_ad_events = ad_events;

  const auto iter = std::remove_if(
      filtered_ad_events.begin(), filtered_ad_events.end(),
      [&ad](const AdEventInfo& ad_event) {
        return (ad_event.type != AdType::kAdNotification &&
                ad_event.type != AdType::kInlineContentAd) ||
               ad_event.creative_set_id != ad.creative_set_id ||
               ad_event.confirmation_type != ConfirmationType::kConversion;
      });

  filtered_ad_events.erase(iter, filtered_ad_events.end());

  return filtered_ad_events;
}

}  // namespace ads
