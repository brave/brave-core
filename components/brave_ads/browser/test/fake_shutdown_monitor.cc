/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/test/fake_shutdown_monitor.h"

#include <utility>

namespace brave_ads::test {

FakeShutdownMonitor::FakeShutdownMonitor() = default;

FakeShutdownMonitor::~FakeShutdownMonitor() = default;

base::CallbackListSubscription FakeShutdownMonitor::AddAppTerminatingCallback(
    base::OnceClosure callback) {
  return app_terminating_callbacks_.Add(std::move(callback));
}

void FakeShutdownMonitor::NotifyAppTerminating() {
  app_terminating_callbacks_.Notify();
}

}  // namespace brave_ads::test
