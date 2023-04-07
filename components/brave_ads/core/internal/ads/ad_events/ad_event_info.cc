/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/ad_events/ad_event_info.h"

namespace brave_ads {

AdEventInfo::AdEventInfo() = default;

AdEventInfo::AdEventInfo(const AdEventInfo& other) = default;

AdEventInfo& AdEventInfo::operator=(const AdEventInfo& other) = default;

AdEventInfo::AdEventInfo(AdEventInfo&& other) noexcept = default;

AdEventInfo& AdEventInfo::operator=(AdEventInfo&& other) noexcept = default;

AdEventInfo::~AdEventInfo() = default;

}  // namespace brave_ads
