/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_LEGACY_MIGRATION_NOTIFICATIONS_LEGACY_NOTIFICATION_JSON_READER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_LEGACY_MIGRATION_NOTIFICATIONS_LEGACY_NOTIFICATION_JSON_READER_H_

#include <string>

#include "base/containers/circular_deque.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads {

struct NotificationAdInfo;

namespace json::reader {

absl::optional<base::circular_deque<NotificationAdInfo>> ReadNotificationAds(
    const std::string& json);

}  // namespace json::reader
}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_LEGACY_MIGRATION_NOTIFICATIONS_LEGACY_NOTIFICATION_JSON_READER_H_
