/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/application_state/application_state_monitor/application_state_monitor_linux.h"

// Something in (or included in) chrome/browser/ui/browser.h causes a build
// error when ui/base/x/x11_util.h is included after it:
// ../../ui/base/x/x11_util.h:367:68: error: invalid operands to binary
// expression ('x11::EventMask' and 'x11::EventMask')
//  367 |     x11::EventMask event_mask = x11::EventMask::SubstructureNotify |
//      |                                 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ^
//  368 |                                 x11::EventMask::SubstructureRedirect);
//      |                                 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// clang-format off
#include "ui/base/x/x11_util.h"
// clang-format on

#include "base/functional/bind.h"
#include "base/no_destructor.h"
#include "base/task/sequenced_task_runner.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/browser_window/public/global_browser_collection.h"
#include "ui/aura/window.h"
#include "ui/aura/window_tree_host.h"
#include "ui/gfx/x/atom_cache.h"
#include "ui/gfx/x/connection.h"

namespace brave_ads {

// static
ApplicationStateMonitor* ApplicationStateMonitor::GetInstance() {
  static base::NoDestructor<ApplicationStateMonitorLinux> instance;
  return instance.get();
}

ApplicationStateMonitorLinux::ApplicationStateMonitorLinux() {
  BrowserList::AddObserver(this);
  OnBrowserSetLastActive(nullptr);
}

ApplicationStateMonitorLinux::~ApplicationStateMonitorLinux() {
  BrowserList::RemoveObserver(this);
}

bool ApplicationStateMonitorLinux::IsBrowserActive() const {
  x11::Window x11_window = x11::Window::None;
  x11::Connection::Get()->GetPropertyAs(
      ui::GetX11RootWindow(), x11::GetAtom("_NET_ACTIVE_WINDOW"), &x11_window);

  bool found_foreground = false;
  GlobalBrowserCollection::GetInstance()->ForEach(
      [&found_foreground, x11_window](BrowserWindowInterface* browser) {
        auto* window =
            browser->GetBrowserForMigrationOnly()->window()->GetNativeWindow();
        if (!window) {
          return true;
        }

        auto* host = window->GetHost();
        if (!host) {
          return true;
        }

        auto accelerated_widget = host->GetAcceleratedWidget();
        found_foreground =
            x11_window == static_cast<x11::Window>(accelerated_widget);

        return !found_foreground;
      });
  return found_foreground;
}

void ApplicationStateMonitorLinux::OnBrowserSetLastActive(
    Browser* /*browser*/) {
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE,
      base::BindOnce(
          &ApplicationStateMonitorLinux::NotifyBrowserDidBecomeActive,
          weak_ptr_factory_.GetWeakPtr()));
}

void ApplicationStateMonitorLinux::OnBrowserNoLongerActive(
    Browser* /*browser*/) {
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE,
      base::BindOnce(
          &ApplicationStateMonitorLinux::NotifyBrowserDidResignActive,
          weak_ptr_factory_.GetWeakPtr()));
}

}  // namespace brave_ads
