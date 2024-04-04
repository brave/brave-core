/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/metrics/pref_names.h"
#include "brave/browser/ui/browser_dialogs.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/ui/session_crashed_bubble.h"
#include "components/metrics/metrics_pref_names.h"
#include "components/prefs/pref_service.h"

#define ShowIfNotOffTheRecordProfile ShowIfNotOffTheRecordProfile_ChromiumImpl
#include "src/chrome/browser/ui/views/session_crashed_bubble_view.cc"
#undef ShowIfNotOffTheRecordProfile

// static
bool SessionCrashedBubble::ShouldShowCrashReportPermissionAskDialog() {
  PrefService* local_prefs = g_browser_process->local_state();
  if (local_prefs->GetBoolean(metrics::prefs::kDontAskForCrashReporting) ||
      IsMetricsReportingPolicyManaged() ||
      local_prefs->GetBoolean(metrics::prefs::kMetricsReportingEnabled)) {
    return false;
  }

  return true;
}

// static
void SessionCrashedBubble::ShowIfNotOffTheRecordProfile(
    Browser* browser,
    bool skip_tab_checking) {
  // If crash report permission ask dialog is launched, tab restore bubble will
  // be shown after closing aks dialog.
  if (ShouldShowCrashReportPermissionAskDialog()) {
    brave::ShowCrashReportPermissionAskDialog(browser);
    return;
  }

  ShowIfNotOffTheRecordProfile(browser, skip_tab_checking);
}
