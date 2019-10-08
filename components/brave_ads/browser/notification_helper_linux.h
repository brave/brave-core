/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_NOTIFICATION_HELPER_LINUX_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_NOTIFICATION_HELPER_LINUX_H_

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "base/memory/weak_ptr.h"

#include "brave/components/brave_ads/browser/notification_helper.h"

namespace brave_ads {

class NotificationHelperLinux :
    public NotificationHelper,
    public base::SupportsWeakPtr<NotificationHelperLinux> {
 public:
  NotificationHelperLinux();
  ~NotificationHelperLinux() override;

  static NotificationHelperLinux* GetInstance();

 private:
  // NotificationHelper impl
  bool ShouldShowNotifications() const override;

  bool ShowMyFirstAdNotification() const override;

  bool CanShowBackgroundNotifications() const override;

  friend struct base::DefaultSingletonTraits<NotificationHelperLinux>;
  DISALLOW_COPY_AND_ASSIGN(NotificationHelperLinux);
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_NOTIFICATION_HELPER_LINUX_H_
