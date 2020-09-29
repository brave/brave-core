/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_NOTIFICATION_HELPER_ANDROID_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_NOTIFICATION_HELPER_ANDROID_H_

#include <memory>

#include "base/memory/weak_ptr.h"

#include "brave/components/brave_ads/browser/notification_helper.h"

namespace brave_ads {

class NotificationHelperAndroid
    : public NotificationHelper,
      public base::SupportsWeakPtr<NotificationHelperAndroid> {
 public:
  NotificationHelperAndroid(const NotificationHelperAndroid&) = delete;
  NotificationHelperAndroid& operator=(
      const NotificationHelperAndroid&) = delete;

  static NotificationHelperAndroid* GetInstanceImpl();

 private:
  friend struct base::DefaultSingletonTraits<NotificationHelperAndroid>;

  NotificationHelperAndroid();
  ~NotificationHelperAndroid() override;

  int GetOperatingSystemVersion() const;

  bool ShowMyFirstAdNotification() override;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_NOTIFICATION_HELPER_ANDROID_H_
