/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_AD_NOTIFICATION_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_AD_NOTIFICATION_H_

#include <memory>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "build/build_config.h"

namespace brave_custom_notification {
class Notification;
}

namespace ads {
struct AdNotificationInfo;
}

namespace brave_ads {

// On Android, this URL must represent an HTTP or HTTPS web origin.
const char* const kBraveAdsUrlPrefix = "https://www.brave.com/ads/?";

std::unique_ptr<brave_custom_notification::Notification> CreateAdNotification(
    const ads::AdNotificationInfo& info);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_AD_NOTIFICATION_H_
