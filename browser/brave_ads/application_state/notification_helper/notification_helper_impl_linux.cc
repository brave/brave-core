/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/application_state/notification_helper/notification_helper_impl_linux.h"

#include "base/feature_list.h"
#include "base/logging.h"
#include "chrome/common/chrome_features.h"

namespace brave_ads {

NotificationHelperImplLinux::NotificationHelperImplLinux() = default;

NotificationHelperImplLinux::~NotificationHelperImplLinux() = default;

bool NotificationHelperImplLinux::CanShowNotifications() {
  // TODO(https://github.com/brave/brave-browser/issues/5542): Investigate how
  // to detect if notifications are enabled within the Linux operating system

  if (!base::FeatureList::IsEnabled(::features::kNativeNotifications)) {
    LOG(WARNING) << "Native notifications feature is disabled";
    return false;
  }

  return true;
}

bool NotificationHelperImplLinux::
    CanShowSystemNotificationsWhileBrowserIsBackgrounded() const {
  return true;
}

bool NotificationHelperImplLinux::ShowOnboardingNotification() {
  return false;
}

}  // namespace brave_ads
