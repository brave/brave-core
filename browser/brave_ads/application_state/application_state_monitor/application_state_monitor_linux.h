/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_ADS_APPLICATION_STATE_APPLICATION_STATE_MONITOR_APPLICATION_STATE_MONITOR_LINUX_H_
#define BRAVE_BROWSER_BRAVE_ADS_APPLICATION_STATE_APPLICATION_STATE_MONITOR_APPLICATION_STATE_MONITOR_LINUX_H_

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/browser/application_state/application_state_monitor.h"
#include "chrome/browser/ui/browser_list_observer.h"

namespace brave_ads {

class ApplicationStateMonitorLinux final : public ApplicationStateMonitor,
                                           public BrowserListObserver {
 public:
  ApplicationStateMonitorLinux();

  ApplicationStateMonitorLinux(const ApplicationStateMonitorLinux&) = delete;
  ApplicationStateMonitorLinux& operator=(const ApplicationStateMonitorLinux&) =
      delete;

  ~ApplicationStateMonitorLinux() override;

 private:
  // BrowserListObserver:
  void OnBrowserSetLastActive(Browser* browser) override;
  void OnBrowserNoLongerActive(Browser* browser) override;

  // ApplicationStateMonitor:
  bool IsBrowserActive() const override;

  base::WeakPtrFactory<ApplicationStateMonitorLinux> weak_ptr_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_BRAVE_ADS_APPLICATION_STATE_APPLICATION_STATE_MONITOR_APPLICATION_STATE_MONITOR_LINUX_H_
