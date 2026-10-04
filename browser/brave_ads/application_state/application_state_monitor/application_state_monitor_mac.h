/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_ADS_APPLICATION_STATE_APPLICATION_STATE_MONITOR_APPLICATION_STATE_MONITOR_MAC_H_
#define BRAVE_BROWSER_BRAVE_ADS_APPLICATION_STATE_APPLICATION_STATE_MONITOR_APPLICATION_STATE_MONITOR_MAC_H_

#include <memory>

#include "brave/components/brave_ads/browser/application_state/application_state_monitor.h"

namespace brave_ads {

class ApplicationStateMonitorMac final : public ApplicationStateMonitor {
 public:
  ApplicationStateMonitorMac();

  ApplicationStateMonitorMac(const ApplicationStateMonitorMac&) = delete;
  ApplicationStateMonitorMac& operator=(const ApplicationStateMonitorMac&) =
      delete;

  ~ApplicationStateMonitorMac() override;

  // Called when the browser becomes active.
  void OnBrowserDidBecomeActive();

  // Called when the browser resigns active.
  void OnBrowserDidResignActive();

 private:
  // ApplicationStateMonitor:
  bool IsBrowserActive() const override;

  class Delegate;
  std::unique_ptr<Delegate> delegate_;
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_BRAVE_ADS_APPLICATION_STATE_APPLICATION_STATE_MONITOR_APPLICATION_STATE_MONITOR_MAC_H_
