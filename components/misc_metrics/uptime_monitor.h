/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_MISC_METRICS_UPTIME_MONITOR_H_
#define BRAVE_COMPONENTS_MISC_METRICS_UPTIME_MONITOR_H_

#include "base/memory/weak_ptr.h"
#include "base/time/time.h"

namespace misc_metrics {

// Interface for uptime monitoring functionality
class UptimeMonitor {
 public:
  virtual ~UptimeMonitor() = default;

  // Returns the total browser usage time for the current week
  virtual base::TimeDelta GetUsedTimeInWeek() const = 0;

  // Returns a weak pointer to this instance
  virtual base::WeakPtr<UptimeMonitor> GetWeakPtr() = 0;

  // Returns true if browser is currently considered to be in use
  virtual bool IsInUse() const = 0;
};

}  // namespace misc_metrics

#endif  // BRAVE_COMPONENTS_MISC_METRICS_UPTIME_MONITOR_H_
