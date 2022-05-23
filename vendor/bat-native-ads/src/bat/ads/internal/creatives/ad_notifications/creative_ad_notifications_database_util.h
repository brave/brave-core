/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CREATIVES_AD_NOTIFICATIONS_CREATIVE_AD_NOTIFICATIONS_DATABASE_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CREATIVES_AD_NOTIFICATIONS_CREATIVE_AD_NOTIFICATIONS_DATABASE_UTIL_H_

#include "bat/ads/internal/creatives/ad_notifications/creative_ad_notification_info_aliases.h"

namespace ads {
namespace database {

void DeleteCreativeAdNotifications();

void SaveCreativeAdNotifications(
    const CreativeAdNotificationList& creative_ads);

}  // namespace database
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CREATIVES_AD_NOTIFICATIONS_CREATIVE_AD_NOTIFICATIONS_DATABASE_UTIL_H_
