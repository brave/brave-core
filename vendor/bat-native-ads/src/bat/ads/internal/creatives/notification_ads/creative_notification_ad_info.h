/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CREATIVES_NOTIFICATION_ADS_CREATIVE_NOTIFICATION_AD_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CREATIVES_NOTIFICATION_ADS_CREATIVE_NOTIFICATION_AD_INFO_H_

#include <string>

#include "bat/ads/internal/creatives/creative_ad_info.h"

namespace ads {

struct CreativeNotificationAdInfo final : CreativeAdInfo {
  CreativeNotificationAdInfo();
  explicit CreativeNotificationAdInfo(const CreativeAdInfo& creative_ad);
  CreativeNotificationAdInfo(
      const CreativeNotificationAdInfo& creative_ad_notification);
  CreativeNotificationAdInfo& operator=(
      const CreativeNotificationAdInfo& creative_ad_notification);
  ~CreativeNotificationAdInfo();

  bool operator==(const CreativeNotificationAdInfo& rhs) const;
  bool operator!=(const CreativeNotificationAdInfo& rhs) const;

  std::string title;
  std::string body;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CREATIVES_NOTIFICATION_ADS_CREATIVE_NOTIFICATION_AD_INFO_H_
