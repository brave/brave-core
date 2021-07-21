/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/metrics/metrics_reporting_util.h"
#include "brave/browser/ui/browser_dialogs.h"
#include "chrome/browser/ui/session_crashed_bubble.h"

// static
void SessionCrashedBubble::ShowIfNotOffTheRecordProfileBrave(Browser* browser) {
  // If crash report permission ask dialog is launched, tab restore bubble will
  // be shown after closing aks dialog.
  if (ShouldShowCrashReportPermissionAskDialog()) {
    brave::ShowCrashReportPermissionAskDialog(browser);
    return;
  }

  ShowIfNotOffTheRecordProfile(browser);
}
