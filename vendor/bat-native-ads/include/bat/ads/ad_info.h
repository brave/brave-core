/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_AD_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_AD_INFO_H_

#include <string>

#include "bat/ads/ad_type.h"
#include "bat/ads/export.h"
#include "url/gurl.h"

namespace ads {

struct ADS_EXPORT AdInfo {
  AdInfo();

  AdInfo(const AdInfo& other);
  AdInfo& operator=(const AdInfo& other);

  AdInfo(AdInfo&& other) noexcept;
  AdInfo& operator=(AdInfo&& other) noexcept;

  ~AdInfo();

  bool operator==(const AdInfo& other) const;
  bool operator!=(const AdInfo& other) const;

  bool IsValid() const;

  AdType type = AdType::kUndefined;
  std::string placement_id;
  std::string creative_instance_id;
  std::string creative_set_id;
  std::string campaign_id;
  std::string advertiser_id;
  std::string segment;
  GURL target_url;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_AD_INFO_H_
