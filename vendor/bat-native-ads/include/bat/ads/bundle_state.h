/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_BUNDLE_STATE_H_
#define BAT_ADS_BUNDLE_STATE_H_

#include <stdint.h>
#include <string>
#include <vector>

#include "bat/ads/creative_ad_notification_info.h"
#include "bat/ads/creative_publisher_ad_info.h"
#include "bat/ads/ad_conversion_tracking_info.h"

namespace ads {

struct BundleState {
  BundleState();
  BundleState(
      const BundleState& state);
  ~BundleState();

  const std::string ToJson() const;
  Result FromJson(
      const std::string& json,
      const std::string& json_schema,
      std::string* error_description = nullptr);

  std::string catalog_id;
  uint64_t catalog_version = 0;
  uint64_t catalog_ping = 0;
  uint64_t catalog_last_updated_timestamp_in_seconds = 0;
  CreativeAdNotificationCategories ad_notification_categories;
  CreativePublisherAdCategories publisher_ad_categories;
  AdConversions ad_conversions;
};

}  // namespace ads

#endif  // BAT_ADS_BUNDLE_STATE_H_
