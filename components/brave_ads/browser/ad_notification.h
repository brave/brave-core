/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_AD_NOTIFICATION_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_AD_NOTIFICATION_

#include <memory>
#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "bat/ads/notification_info.h"
#include "build/build_config.h"

namespace message_center {
class Notification;
}

namespace brave_ads {

std::unique_ptr<message_center::Notification> CreateAdNotification(
    const ads::NotificationInfo& notification_info);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_AD_NOTIFICATION_
