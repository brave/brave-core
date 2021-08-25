/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_NOTIFICATION_HELPER_MAC_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_NOTIFICATION_HELPER_MAC_H_

#include "base/memory/weak_ptr.h"

#include "brave/components/brave_ads/browser/notification_helper.h"

namespace brave_ads {

class NotificationHelperMac
    : public NotificationHelper,
      public base::SupportsWeakPtr<NotificationHelperMac> {
 public:
  NotificationHelperMac(const NotificationHelperMac&) = delete;
  NotificationHelperMac& operator=(const NotificationHelperMac&) = delete;

  static NotificationHelperMac* GetInstanceImpl();

 private:
  friend struct base::DefaultSingletonTraits<NotificationHelperMac>;

  NotificationHelperMac();
  ~NotificationHelperMac() override;

  bool IsEnabled() const;
  bool IsAuthorized() const;

  // NotificationHelper impl
  bool ShouldShowNotifications() override;

  bool CanShowNativeNotifications() override;

  bool ShowMyFirstAdNotification() override;

  bool CanShowBackgroundNotifications() const override;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_NOTIFICATION_HELPER_MAC_H_
