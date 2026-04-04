/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_ADS_APPLICATION_STATE_APPLICATION_STATE_MONITOR_APPLICATION_STATE_MONITOR_WIN_H_
#define BRAVE_BROWSER_BRAVE_ADS_APPLICATION_STATE_APPLICATION_STATE_MONITOR_APPLICATION_STATE_MONITOR_WIN_H_

#include "base/callback_list.h"
#include "base/win/windows_types.h"
#include "brave/components/brave_ads/browser/application_state/application_state_monitor.h"

namespace brave_ads {

class ApplicationStateMonitorWin final : public ApplicationStateMonitor {
 public:
  ApplicationStateMonitorWin();

  ApplicationStateMonitorWin(const ApplicationStateMonitorWin&) = delete;
  ApplicationStateMonitorWin& operator=(const ApplicationStateMonitorWin&) =
      delete;

  ~ApplicationStateMonitorWin() override;

 private:
  void OnWndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

  // ApplicationStateMonitor:
  bool IsBrowserActive() const override;

  base::CallbackListSubscription singleton_hwnd_subscription_;
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_BRAVE_ADS_APPLICATION_STATE_APPLICATION_STATE_MONITOR_APPLICATION_STATE_MONITOR_WIN_H_
