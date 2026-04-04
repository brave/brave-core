/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/application_state/application_state_monitor.h"

#include "brave/components/brave_ads/browser/application_state/application_state_observer.h"

namespace brave_ads {

ApplicationStateMonitor::ApplicationStateMonitor() = default;

ApplicationStateMonitor::~ApplicationStateMonitor() = default;

void ApplicationStateMonitor::AddObserver(
    ApplicationStateObserver* const observer) {
  observers_.AddObserver(observer);
}

void ApplicationStateMonitor::RemoveObserver(
    ApplicationStateObserver* const observer) {
  observers_.RemoveObserver(observer);
}

bool ApplicationStateMonitor::IsBrowserActive() const {
  return true;
}

void ApplicationStateMonitor::NotifyBrowserDidBecomeActive() {
  observers_.Notify(&ApplicationStateObserver::OnBrowserDidBecomeActive);
}

void ApplicationStateMonitor::NotifyBrowserDidResignActive() {
  observers_.Notify(&ApplicationStateObserver::OnBrowserDidResignActive);
}

}  // namespace brave_ads
