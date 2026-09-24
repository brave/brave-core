/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/application_state/application_state_monitor/application_state_monitor_linux.h"

#include "base/functional/bind.h"
#include "base/task/sequenced_task_runner.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/browser_window/public/global_browser_collection.h"

namespace brave_ads {

// static
std::unique_ptr<ApplicationStateMonitor> ApplicationStateMonitor::Create() {
  return std::make_unique<ApplicationStateMonitorLinux>();
}

ApplicationStateMonitorLinux::ApplicationStateMonitorLinux() {
  browser_collection_observation_.Observe(
      GlobalBrowserCollection::GetInstance());
  OnBrowserActivated(nullptr);
}

ApplicationStateMonitorLinux::~ApplicationStateMonitorLinux() = default;

bool ApplicationStateMonitorLinux::IsBrowserActive() const {
  bool is_browser_active = false;
  GlobalBrowserCollection::GetInstance()->ForEach(
      [&is_browser_active](BrowserWindowInterface* browser) {
        is_browser_active = browser->IsActive();
        return !is_browser_active;
      });
  return is_browser_active;
}

void ApplicationStateMonitorLinux::OnBrowserActivated(
    BrowserWindowInterface* /*browser*/) {
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE,
      base::BindOnce(
          &ApplicationStateMonitorLinux::NotifyBrowserDidBecomeActive,
          weak_ptr_factory_.GetWeakPtr()));
}

void ApplicationStateMonitorLinux::OnBrowserDeactivated(
    BrowserWindowInterface* /*browser*/) {
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE,
      base::BindOnce(
          &ApplicationStateMonitorLinux::NotifyBrowserDidResignActive,
          weak_ptr_factory_.GetWeakPtr()));
}

}  // namespace brave_ads
