/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_EVENTS_AD_EVENT_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_EVENTS_AD_EVENT_INFO_H_

#include <cstdint>
#include <string>
#include <vector>

#include "bat/ads/ad_type.h"
#include "bat/ads/confirmation_type.h"

namespace ads {

struct AdEventInfo {
  AdEventInfo();
  AdEventInfo(const AdEventInfo& info);
  ~AdEventInfo();

  AdType type = AdType::kUndefined;
  ConfirmationType confirmation_type = ConfirmationType::kUndefined;
  std::string uuid;
  std::string campaign_id;
  std::string creative_set_id;
  std::string creative_instance_id;
  std::string advertiser_id;
  int64_t timestamp = 0;
};

using AdEventList = std::vector<AdEventInfo>;

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_EVENTS_AD_EVENT_INFO_H_
