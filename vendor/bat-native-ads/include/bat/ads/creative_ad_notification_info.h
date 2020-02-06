/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_CREATIVE_AD_NOTIFICATION_INFO_H_
#define BAT_ADS_CREATIVE_AD_NOTIFICATION_INFO_H_

#include <map>
#include <string>
#include <vector>

#include "bat/ads/creative_ad_info.h"
#include "bat/ads/export.h"
#include "bat/ads/result.h"

namespace ads {

struct ADS_EXPORT CreativeAdNotificationInfo : CreativeAdInfo {
  CreativeAdNotificationInfo();
  CreativeAdNotificationInfo(
      const CreativeAdNotificationInfo& info);
  ~CreativeAdNotificationInfo();

  const std::string ToJson() const;
  Result FromJson(
      const std::string& json,
      std::string* error_description = nullptr);

  std::string title;
  std::string body;
  std::string target_url;
};

using CreativeAdNotifications = std::vector<CreativeAdNotificationInfo>;
using CreativeAdNotificationCategories =
    std::map<std::string, CreativeAdNotifications>;

}  // namespace ads

#endif  // BAT_ADS_CREATIVE_AD_NOTIFICATION_INFO_H_
