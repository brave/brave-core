/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BUNDLE_CREATIVE_AD_NOTIFICATION_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BUNDLE_CREATIVE_AD_NOTIFICATION_INFO_H_

#include <string>

#include "bat/ads/internal/bundle/creative_ad_info.h"

namespace ads {

struct CreativeAdNotificationInfo final : CreativeAdInfo {
  CreativeAdNotificationInfo();
  explicit CreativeAdNotificationInfo(const CreativeAdInfo& creative_ad);
  ~CreativeAdNotificationInfo();

  bool operator==(const CreativeAdNotificationInfo& rhs) const;
  bool operator!=(const CreativeAdNotificationInfo& rhs) const;

  std::string title;
  std::string body;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BUNDLE_CREATIVE_AD_NOTIFICATION_INFO_H_
