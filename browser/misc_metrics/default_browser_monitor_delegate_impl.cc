/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/misc_metrics/default_browser_monitor_delegate_impl.h"

#include "brave/browser/brave_stats/first_run_util.h"
#include "chrome/browser/shell_integration.h"

namespace misc_metrics {

DefaultBrowserMonitorDelegateImpl::DefaultBrowserMonitorDelegateImpl(
    PrefService* local_state)
    : local_state_(local_state) {}

DefaultBrowserMonitorDelegateImpl::~DefaultBrowserMonitorDelegateImpl() =
    default;

bool DefaultBrowserMonitorDelegateImpl::IsDefaultBrowser() {
  auto state = shell_integration::GetDefaultBrowser();
  return state == shell_integration::IS_DEFAULT ||
         state == shell_integration::OTHER_MODE_IS_DEFAULT;
}

bool DefaultBrowserMonitorDelegateImpl::IsFirstRun() {
  return brave_stats::IsFirstRun(local_state_);
}

}  // namespace misc_metrics
