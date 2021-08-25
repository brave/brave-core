/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/metrics/metrics_reporting_util.h"
#include "brave/browser/ui/brave_browser.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/ui/session_crashed_bubble.h"
#include "chrome/browser/ui/test/test_browser_dialog.h"
#include "components/metrics/metrics_pref_names.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"

class CrashReportPermissionAskDialogTest : public DialogBrowserTest {
 public:
  CrashReportPermissionAskDialogTest() = default;
  ~CrashReportPermissionAskDialogTest() override = default;

  // TestBrowserUi:
  void ShowUi(const std::string& name) override {
    SessionCrashedBubble::ShowIfNotOffTheRecordProfileBrave(browser());
  }
};

IN_PROC_BROWSER_TEST_F(CrashReportPermissionAskDialogTest, InvokeUi_Dialog) {
  // When reporting is disabled, crash report permission ask dialog is launched.
  g_browser_process->local_state()->SetBoolean(
      metrics::prefs::kMetricsReportingEnabled, false);
  ShowAndVerifyUi();

  // When reporting is enabled, tab restore dialog is launched.
  g_browser_process->local_state()->SetBoolean(
      metrics::prefs::kMetricsReportingEnabled, true);
  ShowAndVerifyUi();
}
