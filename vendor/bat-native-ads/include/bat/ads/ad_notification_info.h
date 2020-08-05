/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_AD_NOTIFICATION_INFO_H_
#define BAT_ADS_AD_NOTIFICATION_INFO_H_

#include <string>

#include "bat/ads/ad_info.h"
#include "bat/ads/export.h"
#include "bat/ads/result.h"

namespace ads {

struct ADS_EXPORT AdNotificationInfo : AdInfo {
  AdNotificationInfo();
  AdNotificationInfo(
      const AdNotificationInfo& info);
  ~AdNotificationInfo();

  std::string ToJson() const;
  Result FromJson(
      const std::string& json);

  std::string uuid;
  std::string parent_uuid;
  std::string title;
  std::string body;
};

}  // namespace ads

#endif  // BAT_ADS_AD_NOTIFICATION_INFO_H_
