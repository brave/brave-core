/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/application_state/application_state_monitor/application_state_monitor_android.h"

#include <memory>

#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "brave/browser/brave_ads/application_state/application_state_util_android.h"

namespace brave_ads {

// static
std::unique_ptr<ApplicationStateMonitor> ApplicationStateMonitor::Create() {
  return std::make_unique<ApplicationStateMonitorAndroid>();
}

ApplicationStateMonitorAndroid::ApplicationStateMonitorAndroid() {
  application_status_listener_ =
      base::android::ApplicationStatusListener::New(base::BindRepeating(
          &ApplicationStateMonitorAndroid::OnApplicationStateChange,
          weak_ptr_factory_.GetWeakPtr()));

  application_state_ = base::android::ApplicationStatusListener::GetState();
}

ApplicationStateMonitorAndroid::~ApplicationStateMonitorAndroid() = default;

bool ApplicationStateMonitorAndroid::IsBrowserActive() const {
  return IsApplicationInForeground();
}

void ApplicationStateMonitorAndroid::OnApplicationStateChange(
    base::android::ApplicationState state) {
  if (state == base::android::APPLICATION_STATE_HAS_RUNNING_ACTIVITIES) {
    NotifyBrowserDidBecomeActive();
  } else if (application_state_ != state &&
             application_state_ ==
                 base::android::APPLICATION_STATE_HAS_RUNNING_ACTIVITIES) {
    NotifyBrowserDidResignActive();
  }

  application_state_ = state;
}

}  // namespace brave_ads
