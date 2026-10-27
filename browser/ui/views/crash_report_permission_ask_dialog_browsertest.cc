/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/command_line.h"
#include "brave/browser/metrics/metrics_reporting_util.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profile_test_util.h"
#include "chrome/browser/sessions/exit_type_service.h"
#include "chrome/browser/ui/session_crashed_bubble.h"
#include "chrome/browser/ui/startup/infobar_utils.h"
#include "chrome/browser/ui/startup/startup_types.h"
#include "chrome/browser/ui/test/test_browser_dialog.h"
#include "components/metrics/metrics_pref_names.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"
#include "ui/views/widget/any_widget_observer.h"

class CrashReportPermissionAskDialogTest : public DialogBrowserTest {
 public:
  CrashReportPermissionAskDialogTest() = default;
  ~CrashReportPermissionAskDialogTest() override = default;

  // TestBrowserUi:
  void ShowUi(const std::string& name) override {
    ShowSessionCrashedBubbleIfNotOffTheRecordProfileBrave(
        browser(), /*skip_tab_checking=*/false);
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

// The startup path reaches the session crashed bubble through
// AddInfoBarsIfNecessary(), which must ask for crash report permission first.
IN_PROC_BROWSER_TEST_F(CrashReportPermissionAskDialogTest,
                       ShownFromStartupInfoBars) {
  g_browser_process->local_state()->SetBoolean(
      metrics::prefs::kMetricsReportingEnabled, false);

  // A profile whose browser wasn't opened by StartupBrowserCreator, so its
  // crashed last session still counts as a pending unclean exit.
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  Profile& profile = profiles::testing::CreateProfileSync(
      profile_manager, profile_manager->GenerateNextProfileDirectoryPath());
  Browser* crashed_browser = CreateBrowser(&profile);
  ExitTypeService::GetInstanceForProfile(&profile)
      ->SetLastSessionExitTypeForTest(ExitType::kCrashed);

  views::NamedWidgetShownWaiter waiter(views::test::AnyWidgetTestPasskey{},
                                       "CrashReportPermissionAskDialogView");
  AddInfoBarsIfNecessary(crashed_browser, &profile,
                         *base::CommandLine::ForCurrentProcess(),
                         chrome::startup::IsFirstRun::kNo,
                         /*is_web_app=*/false, /*is_post_crash_launch=*/false,
                         /*was_restarted=*/false);
  EXPECT_TRUE(waiter.WaitIfNeededAndGet());
}
