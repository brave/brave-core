/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_NOTIFICATION_HELPER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_NOTIFICATION_HELPER_H_

#include <string>

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "build/build_config.h"

namespace brave_ads {

class NotificationHelper {
 public:
  static NotificationHelper* GetInstance();

  virtual bool ShouldShowNotifications() const;

  virtual bool ShowMyFirstAdNotification() const;

  virtual bool CanShowBackgroundNotifications() const;

 protected:
  NotificationHelper();
  virtual ~NotificationHelper();

 private:
  friend struct base::DefaultSingletonTraits<NotificationHelper>;
  DISALLOW_COPY_AND_ASSIGN(NotificationHelper);
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_NOTIFICATION_HELPER_H_
