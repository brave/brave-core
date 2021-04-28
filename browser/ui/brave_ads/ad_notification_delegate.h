/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_BRAVE_ADS_AD_NOTIFICATION_DELEGATE_H_
#define BRAVE_BROWSER_UI_BRAVE_ADS_AD_NOTIFICATION_DELEGATE_H_

#include "base/memory/ref_counted.h"

namespace brave_ads {

class NotificationObserver {
 public:
  // Called when the notification is shown
  virtual void OnShow() {}

  // Called when the notification is closed. If closed by a user explicitly
  // then |by_user| should be true, otherwise false
  virtual void OnClose(const bool by_user) {}

  // Called when the notification is clicked
  virtual void OnClick() {}
};

// Ref counted version of NotificationObserver, required to satisfy
// brave_ads::AdNotification::delegate_
class AdNotificationDelegate
    : public NotificationObserver,
      public base::RefCountedThreadSafe<AdNotificationDelegate> {
 protected:
  virtual ~AdNotificationDelegate() = default;

 private:
  friend class base::RefCountedThreadSafe<AdNotificationDelegate>;
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_UI_BRAVE_ADS_AD_NOTIFICATION_DELEGATE_H_
