/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_ADS_APPLICATION_STATE_SHUTDOWN_MONITOR_IMPL_H_
#define BRAVE_BROWSER_BRAVE_ADS_APPLICATION_STATE_SHUTDOWN_MONITOR_IMPL_H_

#include "brave/components/brave_ads/browser/application_state/shutdown_monitor.h"

namespace brave_ads {

// Desktop implementation of `ShutdownMonitor`, which chrome/browser/ code can
// depend on but components/ code cannot.
class ShutdownMonitorImpl final : public ShutdownMonitor {
 public:
  ShutdownMonitorImpl() = default;

  ShutdownMonitorImpl(const ShutdownMonitorImpl&) = delete;
  ShutdownMonitorImpl& operator=(const ShutdownMonitorImpl&) = delete;

  ~ShutdownMonitorImpl() override = default;

  // ShutdownMonitor:
  base::CallbackListSubscription AddAppTerminatingCallback(
      base::OnceClosure callback) override;
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_BRAVE_ADS_APPLICATION_STATE_SHUTDOWN_MONITOR_IMPL_H_
