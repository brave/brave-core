/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/metrics/metrics_util.h"

#include "brave/browser/metrics/pref_names.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/metrics/chrome_metrics_service_accessor.h"
#include "chrome/browser/metrics/metrics_reporting_state.h"
#include "components/prefs/pref_service.h"

namespace metrics {

bool ShouldShowCrashReportPermissionAskDialog() {
  PrefService* local_prefs = g_browser_process->local_state();
  if (local_prefs->GetBoolean(prefs::kDontAskForCrashReporting) ||
      IsMetricsReportingPolicyManaged() ||
      ChromeMetricsServiceAccessor::IsMetricsAndCrashReportingEnabled()) {
    return false;
  }

  return true;
}

}  // namespace metrics
