/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/application_state/application_state_monitor/application_state_monitor_win.h"

#include <windows.h>

#include "base/functional/bind.h"
#include "base/no_destructor.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface_iterator.h"
#include "ui/gfx/win/singleton_hwnd.h"
#include "ui/views/win/hwnd_util.h"

namespace brave_ads {

// static
ApplicationStateMonitor* ApplicationStateMonitor::GetInstance() {
  static base::NoDestructor<ApplicationStateMonitorWin> instance;
  return instance.get();
}

ApplicationStateMonitorWin::ApplicationStateMonitorWin() {
  singleton_hwnd_subscription_ =
      gfx::SingletonHwnd::GetInstance()->RegisterCallback(base::BindRepeating(
          &ApplicationStateMonitorWin::OnWndProc, base::Unretained(this)));
}

ApplicationStateMonitorWin::~ApplicationStateMonitorWin() = default;

bool ApplicationStateMonitorWin::IsBrowserActive() const {
  auto* browser = GetLastActiveBrowserWindowInterfaceWithAnyProfile();
  if (browser && browser->GetWindow()) {
    return ::GetForegroundWindow() ==
           views::HWNDForNativeWindow(browser->GetWindow()->GetNativeWindow());
  }

  return false;
}

void ApplicationStateMonitorWin::OnWndProc(HWND hwnd,
                                           UINT message,
                                           WPARAM wparam,
                                           LPARAM lparam) {
  if (message != WM_ACTIVATEAPP) {
    return;
  }

  if ((BOOL)wparam) {
    NotifyBrowserDidBecomeActive();
  } else {
    NotifyBrowserDidResignActive();
  }
}

}  // namespace brave_ads
