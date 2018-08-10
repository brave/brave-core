/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_PROFILE_CREATION_MONITOR_H_
#define BRAVE_BROWSER_PROFILE_CREATION_MONITOR_H_

#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"

class ProfileCreationMonitor : public content::NotificationObserver {
 public:
  ProfileCreationMonitor();
  ~ProfileCreationMonitor() override;

 private:
  // content::NotificationObserver overrides:
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

  content::NotificationRegistrar registrar_;

  DISALLOW_COPY_AND_ASSIGN(ProfileCreationMonitor);
};

#endif  // BRAVE_BROWSER_PROFILE_CREATION_MONITOR_H_
