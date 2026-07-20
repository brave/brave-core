/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_APPLICATION_STATE_SHUTDOWN_MONITOR_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_APPLICATION_STATE_SHUTDOWN_MONITOR_H_

#include "base/callback_list.h"
#include "base/functional/callback_forward.h"

namespace brave_ads {

// Runs registered callbacks once the browser has started terminating.
class ShutdownMonitor {
 public:
  ShutdownMonitor() = default;

  ShutdownMonitor(const ShutdownMonitor&) = delete;
  ShutdownMonitor& operator=(const ShutdownMonitor&) = delete;

  virtual ~ShutdownMonitor() = default;

  virtual base::CallbackListSubscription AddAppTerminatingCallback(
      base::OnceClosure callback) = 0;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_APPLICATION_STATE_SHUTDOWN_MONITOR_H_
