/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_FREQUENCY_CAPPING_FREQUENCY_CAPPING_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_FREQUENCY_CAPPING_FREQUENCY_CAPPING_UTIL_H_

#include <cstdint>
#include <deque>

#include "bat/ads/internal/ad_events/ad_event_info.h"

namespace ads {

std::deque<uint64_t> GetTimestampHistoryForAdEvents(
    const AdEventList& ad_events);

bool DoesHistoryRespectCapForRollingTimeConstraint(
    const std::deque<uint64_t>& history,
    const uint64_t time_constraint_in_seconds,
    const uint64_t cap);

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_FREQUENCY_CAPPING_FREQUENCY_CAPPING_UTIL_H_
