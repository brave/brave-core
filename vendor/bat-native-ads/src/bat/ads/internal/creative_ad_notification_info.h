/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_CREATIVE_AD_NOTIFICATION_INFO_H_
#define BAT_ADS_INTERNAL_CREATIVE_AD_NOTIFICATION_INFO_H_

#include "bat/ads/internal/creative_ad_info.h"

#include <string>
#include <vector>

namespace ads {

struct CreativeAdNotificationInfo : CreativeAdInfo {
  CreativeAdNotificationInfo();
  ~CreativeAdNotificationInfo();

  bool operator==(
      const CreativeAdNotificationInfo& rhs) const;

  bool operator!=(
      const CreativeAdNotificationInfo& rhs) const;

  std::string title;
  std::string body;
};

using CreativeAdNotificationList = std::vector<CreativeAdNotificationInfo>;

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_CREATIVE_AD_NOTIFICATION_INFO_H_
