/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/bundle/creative_ad_notification_info.h"

namespace ads {

CreativeAdNotificationInfo::CreativeAdNotificationInfo() = default;

CreativeAdNotificationInfo::CreativeAdNotificationInfo(
    const CreativeAdInfo& creative_ad)
    : CreativeAdInfo(creative_ad) {}

CreativeAdNotificationInfo::~CreativeAdNotificationInfo() = default;

bool CreativeAdNotificationInfo::operator==(
    const CreativeAdNotificationInfo& rhs) const {
  return title == rhs.title && body == rhs.body;
}

bool CreativeAdNotificationInfo::operator!=(
    const CreativeAdNotificationInfo& rhs) const {
  return !(*this == rhs);
}

}  // namespace ads
