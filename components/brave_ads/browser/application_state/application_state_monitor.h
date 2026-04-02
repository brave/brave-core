/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_APPLICATION_STATE_APPLICATION_STATE_MONITOR_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_APPLICATION_STATE_APPLICATION_STATE_MONITOR_H_

#include "base/observer_list.h"
#include "brave/components/brave_ads/browser/application_state/application_state_observer.h"

namespace brave_ads {

// Notifies `ApplicationStateObserver` when the browser becomes active or
// inactive.
class ApplicationStateMonitor {
 public:
  ApplicationStateMonitor();

  ApplicationStateMonitor(const ApplicationStateMonitor&) = delete;
  ApplicationStateMonitor& operator=(const ApplicationStateMonitor&) = delete;

  virtual ~ApplicationStateMonitor();

  // Returns the singleton instance.
  static ApplicationStateMonitor* GetInstance();

  void AddObserver(ApplicationStateObserver* observer);
  void RemoveObserver(ApplicationStateObserver* observer);

  // Returns whether the browser is currently active.
  virtual bool IsBrowserActive() const;

 protected:
  // Notifies observers that the browser became active.
  void NotifyBrowserDidBecomeActive();

  // Notifies observers that the browser resigned active.
  void NotifyBrowserDidResignActive();

 private:
  base::ObserverList<ApplicationStateObserver> observers_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_APPLICATION_STATE_APPLICATION_STATE_MONITOR_H_
