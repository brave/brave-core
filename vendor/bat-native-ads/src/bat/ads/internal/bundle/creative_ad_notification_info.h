/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BUNDLE_CREATIVE_AD_NOTIFICATION_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BUNDLE_CREATIVE_AD_NOTIFICATION_INFO_H_

#include <string>
#include <vector>

#include "bat/ads/internal/bundle/creative_ad_info.h"

namespace ads {

struct CreativeAdNotificationInfo : CreativeAdInfo {
  CreativeAdNotificationInfo();
  ~CreativeAdNotificationInfo();

  bool operator==(const CreativeAdNotificationInfo& rhs) const;

  bool operator!=(const CreativeAdNotificationInfo& rhs) const;

  std::string title;
  std::string body;
};

using CreativeAdNotificationList = std::vector<CreativeAdNotificationInfo>;

CreativeAdList ToCreativeAdList(
    const CreativeAdNotificationList& creative_ad_notifications) {
  CreativeAdList creative_ads;

  for (const auto& creative_ad_notification : creative_ad_notifications) {
    const CreativeAdInfo creative_ad =
        static_cast<CreativeAdInfo>(creative_ad_notification);
    creative_ads.push_back(creative_ad);
  }

  return creative_ads;
}

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BUNDLE_CREATIVE_AD_NOTIFICATION_INFO_H_
