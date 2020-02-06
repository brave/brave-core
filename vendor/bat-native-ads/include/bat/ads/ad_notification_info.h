/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_AD_NOTIFICATION_INFO_H_
#define BAT_ADS_AD_NOTIFICATION_INFO_H_

#include <string>

#include "bat/ads/export.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/result.h"

namespace ads {

struct ADS_EXPORT AdNotificationInfo {
  AdNotificationInfo();
  AdNotificationInfo(
      const AdNotificationInfo& info);
  ~AdNotificationInfo();

  const std::string ToJson() const;
  Result FromJson(
      const std::string& json,
      std::string* error_description = nullptr);

  std::string uuid;
  std::string creative_instance_id;
  std::string creative_set_id;
  std::string category;
  std::string title;
  std::string body;
  std::string target_url;
  ConfirmationType confirmation_type = ConfirmationType::kUnknown;
};

}  // namespace ads

#endif  // BAT_ADS_AD_NOTIFICATION_INFO_H_
