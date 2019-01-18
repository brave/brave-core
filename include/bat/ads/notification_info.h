/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_NOTIFICATION_INFO_H_
#define BAT_ADS_NOTIFICATION_INFO_H_

#include <string>

#include "bat/ads/export.h"
#include "bat/ads/result.h"

namespace ads {

struct ADS_EXPORT NotificationInfo {
  NotificationInfo();
  explicit NotificationInfo(const NotificationInfo& info);
  ~NotificationInfo();

  const std::string ToJson() const;
  Result FromJson(
      const std::string& json,
      std::string* error_description = nullptr);

  std::string creative_set_id;
  std::string category;
  std::string advertiser;
  std::string text;
  std::string url;
  std::string uuid;
};

}  // namespace ads

#endif  // BAT_ADS_NOTIFICATION_INFO_H_
