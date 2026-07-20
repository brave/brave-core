/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/application_state/shutdown_monitor_impl.h"

#include <utility>

#include "chrome/browser/lifetime/termination_notification.h"

namespace brave_ads {

base::CallbackListSubscription ShutdownMonitorImpl::AddAppTerminatingCallback(
    base::OnceClosure callback) {
  return browser_shutdown::AddAppTerminatingCallback(std::move(callback));
}

}  // namespace brave_ads
