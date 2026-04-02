/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_ADS_APPLICATION_STATE_APPLICATION_STATE_MONITOR_APPLICATION_STATE_MONITOR_ANDROID_H_
#define BRAVE_BROWSER_BRAVE_ADS_APPLICATION_STATE_APPLICATION_STATE_MONITOR_APPLICATION_STATE_MONITOR_ANDROID_H_

#include <memory>

#include "base/android/application_status_listener.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/browser/application_state/application_state_monitor.h"

namespace brave_ads {

class ApplicationStateMonitorAndroid final : public ApplicationStateMonitor {
 public:
  ApplicationStateMonitorAndroid();

  ApplicationStateMonitorAndroid(const ApplicationStateMonitorAndroid&) =
      delete;
  ApplicationStateMonitorAndroid& operator=(
      const ApplicationStateMonitorAndroid&) = delete;

  ~ApplicationStateMonitorAndroid() override;

 private:
  // ApplicationStatusListener:
  void OnApplicationStateChange(base::android::ApplicationState state);

  // ApplicationStateMonitor:
  bool IsBrowserActive() const override;

  std::unique_ptr<base::android::ApplicationStatusListener>
      application_status_listener_;

  base::android::ApplicationState application_state_;

  base::WeakPtrFactory<ApplicationStateMonitorAndroid> weak_ptr_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_BRAVE_ADS_APPLICATION_STATE_APPLICATION_STATE_MONITOR_APPLICATION_STATE_MONITOR_ANDROID_H_
