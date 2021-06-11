/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_AD_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_AD_INFO_H_

#include <string>

#include "bat/ads/ad_type.h"
#include "bat/ads/export.h"

namespace ads {

struct ADS_EXPORT AdInfo {
  AdInfo();
  AdInfo(const AdInfo& info);
  ~AdInfo();

  bool operator==(const AdInfo& rhs) const;

  bool operator!=(const AdInfo& rhs) const;

  bool IsValid() const;

  AdType type = AdType::kUndefined;
  std::string uuid;
  std::string creative_instance_id;
  std::string creative_set_id;
  std::string campaign_id;
  std::string advertiser_id;
  std::string segment;
  std::string target_url;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_AD_INFO_H_
