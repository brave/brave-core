/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_info.h"

namespace brave_ads {

CreativeNotificationAdInfo::CreativeNotificationAdInfo() = default;

CreativeNotificationAdInfo::CreativeNotificationAdInfo(
    const CreativeAdInfo& creative_ad)
    : CreativeAdInfo(creative_ad) {}

bool CreativeNotificationAdInfo::operator==(
    const CreativeNotificationAdInfo& other) const {
  return CreativeAdInfo::operator==(other) && title == other.title &&
         body == other.body;
}

bool CreativeNotificationAdInfo::operator!=(
    const CreativeNotificationAdInfo& other) const {
  return !(*this == other);
}

}  // namespace brave_ads
