/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/notification_helper_linux.h"

#include "base/feature_list.h"
#include "base/logging.h"
#include "chrome/common/chrome_features.h"

namespace brave_ads {

NotificationHelperLinux::NotificationHelperLinux() = default;

NotificationHelperLinux::~NotificationHelperLinux() = default;

bool NotificationHelperLinux::CanShowNativeNotifications() {
  // TODO(https://github.com/brave/brave-browser/issues/5542): Investigate how
  // to detect if notifications are enabled within the Linux operating system

  if (!base::FeatureList::IsEnabled(::features::kNativeNotifications)) {
    LOG(WARNING) << "Native notifications feature is disabled";
    return false;
  }

  return true;
}

bool NotificationHelperLinux::CanShowBackgroundNotifications() const {
  return true;
}

bool NotificationHelperLinux::ShowMyFirstAdNotification() {
  return false;
}

NotificationHelperLinux* NotificationHelperLinux::GetInstanceImpl() {
  return base::Singleton<NotificationHelperLinux>::get();
}

NotificationHelper* NotificationHelper::GetInstanceImpl() {
  return NotificationHelperLinux::GetInstanceImpl();
}

}  // namespace brave_ads
