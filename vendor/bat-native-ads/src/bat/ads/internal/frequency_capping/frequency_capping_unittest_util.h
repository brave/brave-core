/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_FREQUENCY_CAPPING_FREQUENCY_CAPPING_UNITTEST_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_FREQUENCY_CAPPING_FREQUENCY_CAPPING_UNITTEST_UTIL_H_

#include "bat/ads/ad_info.h"
#include "bat/ads/ad_type.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/ad_events/ad_event_info.h"
#include "bat/ads/internal/bundle/creative_ad_info.h"

namespace ads {

AdEventInfo GenerateAdEvent(const AdType& type,
                            const CreativeAdInfo& ad,
                            const ConfirmationType& confirmation_type);

AdEventInfo GenerateAdEvent(const AdType& type,
                            const AdInfo& ad,
                            const ConfirmationType& confirmation_type);

void RecordAdEvents(const AdType& type,
                    const ConfirmationType& confirmation_type,
                    const int count);

void RecordAdEvent(const AdType& type,
                   const ConfirmationType& confirmation_type);

void ResetFrequencyCaps(const AdType& type);

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_FREQUENCY_CAPPING_FREQUENCY_CAPPING_UNITTEST_UTIL_H_
