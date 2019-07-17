/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/notification_helper_win.h"

#include "base/win/core_winrt_util.h"
#include "base/feature_list.h"

namespace brave_ads {

NotificationHelperWin::NotificationHelperWin() = default;

NotificationHelperWin::~NotificationHelperWin() = default;

bool NotificationHelperWin::IsNotificationsAvailable() const {
  if (base::win::GetVersion() < base::win::VERSION_WIN10_RS4 ||
      base::FeatureList::IsEnabled(features::kNativeNotifications)) {
    return true;
  }

  return true;
}

bool NotificationHelperWin::ShowMyFirstAdNotification() const {
  return false;
}

NotificationHelperWin* NotificationHelperWin::GetInstance() {
  return base::Singleton<NotificationHelperWin>::get();
}

NotificationHelper* NotificationHelper::GetInstance() {
  return NotificationHelperWin::GetInstance();
}

}  // namespace brave_ads
