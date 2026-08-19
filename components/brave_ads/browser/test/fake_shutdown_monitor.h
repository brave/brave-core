/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_TEST_FAKE_SHUTDOWN_MONITOR_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_TEST_FAKE_SHUTDOWN_MONITOR_H_

#include "base/callback_list.h"
#include "brave/components/brave_ads/browser/application_state/shutdown_monitor.h"

namespace brave_ads::test {

// Defers `AddAppTerminatingCallback`'s callback until `NotifyAppTerminating`
// is called, letting tests control when the browser appears to quit.
class FakeShutdownMonitor : public ShutdownMonitor {
 public:
  FakeShutdownMonitor();

  FakeShutdownMonitor(const FakeShutdownMonitor&) = delete;
  FakeShutdownMonitor& operator=(const FakeShutdownMonitor&) = delete;

  ~FakeShutdownMonitor() override;

  // ShutdownMonitor:
  base::CallbackListSubscription AddAppTerminatingCallback(
      base::OnceClosure callback) override;

  void NotifyAppTerminating();

 private:
  base::OnceClosureList app_terminating_callbacks_;
};

}  // namespace brave_ads::test

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_TEST_FAKE_SHUTDOWN_MONITOR_H_
