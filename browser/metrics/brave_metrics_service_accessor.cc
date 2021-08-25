/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/metrics/brave_metrics_service_accessor.h"
#include "chrome/browser/browser_process.h"

bool BraveMetricsServiceAccessor::IsMetricsAndCrashReportingEnabled() {
  return IsMetricsReportingEnabled(g_browser_process->local_state());
}
