/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/creatives/notification_ads/creative_notification_ad_info.h"

namespace ads {

CreativeNotificationAdInfo::CreativeNotificationAdInfo() = default;

CreativeNotificationAdInfo::CreativeNotificationAdInfo(
    const CreativeAdInfo& creative_ad)
    : CreativeAdInfo(creative_ad) {}

CreativeNotificationAdInfo::CreativeNotificationAdInfo(
    const CreativeNotificationAdInfo& creative_ad_notification) = default;

CreativeNotificationAdInfo& CreativeNotificationAdInfo::operator=(
    const CreativeNotificationAdInfo& creative_ad_notification) = default;

CreativeNotificationAdInfo::~CreativeNotificationAdInfo() = default;

bool CreativeNotificationAdInfo::operator==(
    const CreativeNotificationAdInfo& rhs) const {
  return CreativeAdInfo::operator==(rhs) && title == rhs.title &&
         body == rhs.body;
}

bool CreativeNotificationAdInfo::operator!=(
    const CreativeNotificationAdInfo& rhs) const {
  return !(*this == rhs);
}

}  // namespace ads
