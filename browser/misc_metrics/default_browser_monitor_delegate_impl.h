/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_MISC_METRICS_DEFAULT_BROWSER_MONITOR_DELEGATE_IMPL_H_
#define BRAVE_BROWSER_MISC_METRICS_DEFAULT_BROWSER_MONITOR_DELEGATE_IMPL_H_

#include "base/memory/raw_ptr.h"
#include "brave/components/misc_metrics/default_browser_monitor.h"

class PrefService;

namespace misc_metrics {

class DefaultBrowserMonitorDelegateImpl
    : public DefaultBrowserMonitor::Delegate {
 public:
  explicit DefaultBrowserMonitorDelegateImpl(PrefService* local_state);
  ~DefaultBrowserMonitorDelegateImpl() override;

  DefaultBrowserMonitorDelegateImpl(const DefaultBrowserMonitorDelegateImpl&) =
      delete;
  DefaultBrowserMonitorDelegateImpl& operator=(
      const DefaultBrowserMonitorDelegateImpl&) = delete;

  // DefaultBrowserMonitor::Delegate:
  bool IsDefaultBrowser() override;
  bool IsFirstRun() override;

 private:
  raw_ptr<PrefService> local_state_;
};

}  // namespace misc_metrics

#endif  // BRAVE_BROWSER_MISC_METRICS_DEFAULT_BROWSER_MONITOR_DELEGATE_IMPL_H_
