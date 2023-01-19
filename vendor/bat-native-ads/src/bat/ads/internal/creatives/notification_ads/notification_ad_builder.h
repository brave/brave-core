/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CREATIVES_NOTIFICATION_ADS_NOTIFICATION_AD_BUILDER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CREATIVES_NOTIFICATION_ADS_NOTIFICATION_AD_BUILDER_H_

#include <string>

namespace ads {

struct CreativeNotificationAdInfo;
struct NotificationAdInfo;

NotificationAdInfo BuildNotificationAd(
    const CreativeNotificationAdInfo& creative_ad);

NotificationAdInfo BuildNotificationAd(
    const CreativeNotificationAdInfo& creative_ad,
    const std::string& placement_id);

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CREATIVES_NOTIFICATION_ADS_NOTIFICATION_AD_BUILDER_H_
