/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/legacy_migration/notifications/legacy_notification_json_reader.h"

#include "base/json/json_reader.h"
#include "base/values.h"
#include "bat/ads/internal/creatives/notification_ads/notification_ad_manager_values_util.h"
#include "bat/ads/notification_ad_info.h"

namespace ads::json::reader {

constexpr char kNotificationListKey[] = "notifications";

absl::optional<base::circular_deque<NotificationAdInfo>> ReadNotificationAds(
    const std::string& json) {
  const absl::optional<base::Value> root = base::JSONReader::Read(json);
  if (!root || !root->is_dict()) {
    return absl::nullopt;
  }

  const base::Value::Dict& dict = root->GetDict();
  const auto* value = dict.FindList(kNotificationListKey);
  if (!value) {
    return absl::nullopt;
  }

  return NotificationAdsFromValue(*value);
}

}  // namespace ads::json::reader
