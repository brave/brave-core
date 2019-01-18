/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_AD_INFO_H_
#define BAT_ADS_AD_INFO_H_

#include <string>
#include <vector>

#include "bat/ads/export.h"
#include "bat/ads/result.h"

namespace ads {

struct ADS_EXPORT AdInfo {
  AdInfo();
  AdInfo(const AdInfo& info);
  ~AdInfo();

  const std::string ToJson() const;
  Result FromJson(
      const std::string& json,
      std::string* error_description = nullptr);

  std::string creative_set_id;
  std::string campaign_id;
  std::string start_timestamp;
  std::string end_timestamp;
  unsigned int daily_cap;
  unsigned int per_day;
  unsigned int total_max;
  std::vector<std::string> regions;
  std::string advertiser;
  std::string notification_text;
  std::string notification_url;
  std::string uuid;
};

}  // namespace ads

#endif  // BAT_ADS_AD_INFO_H_
