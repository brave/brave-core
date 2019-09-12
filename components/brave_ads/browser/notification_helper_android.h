/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_NOTIFICATION_HELPER_ANDROID_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_NOTIFICATION_HELPER_ANDROID_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "base/memory/weak_ptr.h"

#include "brave/components/brave_ads/browser/notification_helper.h"
#include "chrome/browser/notifications/notification_channels_provider_android.h"

namespace brave_ads {

class NotificationHelperAndroid :
    public NotificationHelper,
    public base::SupportsWeakPtr<NotificationHelperAndroid> {
 public:
  NotificationHelperAndroid();
  ~NotificationHelperAndroid() override;

  static NotificationHelperAndroid* GetInstance();

 private:
  bool IsBraveAdsNotificationChannelEnabled() const;

  std::unique_ptr<NotificationChannelsProviderAndroid> channels_provider_ =
      std::make_unique<NotificationChannelsProviderAndroid>();

  int GetOperatingSystemVersion() const;

  // NotificationHelper impl
  bool ShouldShowNotifications() const override;

  bool ShowMyFirstAdNotification() const override;

  bool CanShowBackgroundNotifications() const override;

  friend struct base::DefaultSingletonTraits<NotificationHelperAndroid>;
  DISALLOW_COPY_AND_ASSIGN(NotificationHelperAndroid);
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_NOTIFICATION_HELPER_ANDROID_H_
