/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_EVENTS_AD_EVENTS_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_EVENTS_AD_EVENTS_H_

#include <functional>

#include "bat/ads/result.h"

namespace ads {

using AdEventCallback = std::function<void(const Result)>;

class ConfirmationType;
struct AdEventInfo;
struct AdInfo;

void LogAdEvent(const AdInfo& ad,
                const ConfirmationType& confirmation_type,
                AdEventCallback callback);

void LogAdEvent(const AdEventInfo& ad_event, AdEventCallback callback);

void PurgeExpiredAdEvents(AdEventCallback callback);

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_EVENTS_AD_EVENTS_H_
