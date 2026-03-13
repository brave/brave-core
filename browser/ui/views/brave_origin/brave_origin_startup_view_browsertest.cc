/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/brave_origin/brave_origin_startup_view.h"

#include <optional>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/test/bind.h"
#include "base/test/run_until.h"
#include "brave/components/brave_origin/pref_names.h"
#include "brave/components/skus/browser/pref_names.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/startup/startup_browser_creator.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"

namespace {

// Test delegate that uses real profile infrastructure but records calls to
// OpenExternal and AttemptExit via shared flags (the delegate is moved into
// an async closure during WindowClosing, so raw pointers to it can dangle).
class TestDelegate : public BraveOriginStartupView::Delegate {
 public:
  explicit TestDelegate(bool* attempt_exit_flag)
      : attempt_exit_flag_(attempt_exit_flag) {}

  void OpenExternal(const GURL& url) override {}

  void AttemptExit() override {
    if (attempt_exit_flag_) {
      *attempt_exit_flag_ = true;
    }
  }

  void CreateSystemProfile(
      BraveOriginStartupView::ProfileCallback callback) override {
    g_browser_process->profile_manager()->CreateProfileAsync(
        ProfileManager::GetSystemProfilePath(), std::move(callback));
  }

  void CreateDefaultProfile(
      BraveOriginStartupView::ProfileCallback callback) override {
    g_browser_process->profile_manager()->CreateProfileAsync(
        g_browser_process->profile_manager()->GetLastUsedProfileDir(),
        std::move(callback));
  }

 private:
  raw_ptr<bool> attempt_exit_flag_ = nullptr;
};

}  // namespace

class BraveOriginStartupViewBrowserTest : public InProcessBrowserTest {
 public:
  void SetUp() override {
    BraveOriginStartupView::SetShouldShowDialogForTesting(std::nullopt);
    InProcessBrowserTest::SetUp();
  }

  void TearDown() override {
    InProcessBrowserTest::TearDown();
    BraveOriginStartupView::SetShouldShowDialogForTesting(std::nullopt);
  }

  void TearDownOnMainThread() override {
    if (BraveOriginStartupView::IsShowing()) {
      HideAndWaitForClose();
    }
    InProcessBrowserTest::TearDownOnMainThread();
  }

  // Hide() is a no-op until the widget exists (async profile creation may
  // still be in progress). Retry Hide() inside RunUntil so that pending tasks
  // are processed between attempts, and the widget is closed as soon as it
  // becomes available.
  void HideAndWaitForClose() {
    ASSERT_TRUE(base::test::RunUntil([] {
      BraveOriginStartupView::Hide();
      return !BraveOriginStartupView::IsShowing();
    }));
  }
};

// Verifies the dialog can be shown and IsShowing() reflects its state.
IN_PROC_BROWSER_TEST_F(BraveOriginStartupViewBrowserTest, ShowAndHide) {
  EXPECT_FALSE(BraveOriginStartupView::IsShowing());

  bool on_complete_called = false;
  BraveOriginStartupView::Show(
      base::BindLambdaForTesting([&]() { on_complete_called = true; }),
      std::make_unique<TestDelegate>(nullptr));

  EXPECT_TRUE(BraveOriginStartupView::IsShowing());

  HideAndWaitForClose();

  EXPECT_FALSE(BraveOriginStartupView::IsShowing());
  // Closing without validation should not call on_complete.
  EXPECT_FALSE(on_complete_called);
}

// Verifies that calling Show() twice does not create a second dialog.
IN_PROC_BROWSER_TEST_F(BraveOriginStartupViewBrowserTest, ShowTwiceIsNoOp) {
  BraveOriginStartupView::Show(base::DoNothing(),
                               std::make_unique<TestDelegate>(nullptr));
  EXPECT_TRUE(BraveOriginStartupView::IsShowing());

  // Second Show should be a no-op.
  BraveOriginStartupView::Show(base::DoNothing(),
                               std::make_unique<TestDelegate>(nullptr));
  EXPECT_TRUE(BraveOriginStartupView::IsShowing());

  HideAndWaitForClose();
  EXPECT_FALSE(BraveOriginStartupView::IsShowing());
}

// Verifies that Hide() is safe to call when dialog is not showing.
IN_PROC_BROWSER_TEST_F(BraveOriginStartupViewBrowserTest,
                       HideWhenNotShowingIsNoOp) {
  EXPECT_FALSE(BraveOriginStartupView::IsShowing());
  BraveOriginStartupView::Hide();  // Should not crash.
  EXPECT_FALSE(BraveOriginStartupView::IsShowing());
}

// Verifies that the testing override controls ShouldShowDialog().
IN_PROC_BROWSER_TEST_F(BraveOriginStartupViewBrowserTest,
                       ShouldShowDialogTestingOverride) {
  PrefService* local_state = g_browser_process->local_state();

  // Default in tests: --test-type flag makes it return false.
  EXPECT_FALSE(BraveOriginStartupView::ShouldShowDialog(local_state));

  // Override to force show.
  BraveOriginStartupView::SetShouldShowDialogForTesting(true);
  EXPECT_TRUE(BraveOriginStartupView::ShouldShowDialog(local_state));

  // Override to force hide.
  BraveOriginStartupView::SetShouldShowDialogForTesting(false);
  EXPECT_FALSE(BraveOriginStartupView::ShouldShowDialog(local_state));

  // Reset override.
  BraveOriginStartupView::SetShouldShowDialogForTesting(std::nullopt);
  EXPECT_FALSE(BraveOriginStartupView::ShouldShowDialog(local_state));
}

// Verifies closing without validation calls AttemptExit on the delegate.
IN_PROC_BROWSER_TEST_F(BraveOriginStartupViewBrowserTest,
                       CloseWithoutValidationCallsAttemptExit) {
  bool attempt_exit_called = false;

  BraveOriginStartupView::Show(
      base::DoNothing(), std::make_unique<TestDelegate>(&attempt_exit_called));
  EXPECT_FALSE(attempt_exit_called);

  HideAndWaitForClose();

  EXPECT_FALSE(BraveOriginStartupView::IsShowing());
  EXPECT_TRUE(attempt_exit_called);
}

// Verifies that when the purchase is validated and SKU credentials exist,
// the dialog does not appear and the browser window opens normally.
IN_PROC_BROWSER_TEST_F(BraveOriginStartupViewBrowserTest,
                       PaidUserSkipsDialogAndGetsBrowser) {
  PrefService* local_state = g_browser_process->local_state();

  // Simulate a paid user: set validated pref and add SKU credentials.
  local_state->SetBoolean(brave_origin::kOriginPurchaseValidated, true);
  base::DictValue skus_state;
  skus_state.Set("production", R"({
    "credentials": {
      "items": {
        "origin": "some-credential-value"
      }
    }
  })");
  local_state->SetDict(skus::prefs::kSkusState, std::move(skus_state));

  // With valid purchase state, ShouldShowDialog returns false (even without
  // the --test-type override).
  BraveOriginStartupView::SetShouldShowDialogForTesting(std::nullopt);
  EXPECT_FALSE(BraveOriginStartupView::ShouldShowDialog(local_state));

  // The dialog is not showing, and we have a browser window (provided by the
  // test framework via the normal startup path).
  EXPECT_FALSE(BraveOriginStartupView::IsShowing());
  EXPECT_TRUE(browser() != nullptr);
}

// --------------------------------------------------------------------------
// Integration tests for the StartupBrowserCreator::Start override.
// These call Start() directly to exercise the real startup interception logic
// in chromium_src/chrome/browser/ui/startup/startup_browser_creator.cc.
// --------------------------------------------------------------------------

// Verifies that StartupBrowserCreator::Start() shows the dialog when
// ShouldShowDialog is true, and that closing without validation does not crash.
IN_PROC_BROWSER_TEST_F(BraveOriginStartupViewBrowserTest,
                       StartOverrideShowsDialogOnClose) {
  EXPECT_FALSE(BraveOriginStartupView::IsShowing());

  BraveOriginStartupView::SetShouldShowDialogForTesting(true);

  // Call the real Start() override. It should intercept and show the dialog.
  StartupBrowserCreator browser_creator;
  StartupProfileInfo profile_info{browser()->profile(),
                                  StartupProfileMode::kBrowserWindow};
  bool result = browser_creator.Start(*base::CommandLine::ForCurrentProcess(),
                                      base::FilePath(), profile_info,
                                      {browser()->profile()});
  EXPECT_TRUE(result);
  EXPECT_TRUE(BraveOriginStartupView::IsShowing());

  // Close without validation — should not crash.
  HideAndWaitForClose();
  EXPECT_FALSE(BraveOriginStartupView::IsShowing());
}

// Verifies that StartupBrowserCreator::Start() shows the dialog and that
// simulating validation fires the on_complete callback (which calls
// Start_ChromiumImpl), opening a new browser window without crashing.
IN_PROC_BROWSER_TEST_F(BraveOriginStartupViewBrowserTest,
                       StartOverrideShowsDialogAndProceeds) {
  EXPECT_FALSE(BraveOriginStartupView::IsShowing());

  size_t browser_count_before = chrome::GetTotalBrowserCount();

  BraveOriginStartupView::SetShouldShowDialogForTesting(true);

  StartupBrowserCreator browser_creator;
  StartupProfileInfo profile_info{browser()->profile(),
                                  StartupProfileMode::kBrowserWindow};
  bool result = browser_creator.Start(*base::CommandLine::ForCurrentProcess(),
                                      base::FilePath(), profile_info,
                                      {browser()->profile()});
  EXPECT_TRUE(result);
  EXPECT_TRUE(BraveOriginStartupView::IsShowing());

  // ValidateForTesting() is a no-op until the widget is ready (async profile
  // creation). Retry inside RunUntil so pending tasks are processed between
  // attempts.
  ASSERT_TRUE(base::test::RunUntil([] {
    BraveOriginStartupView::ValidateForTesting();
    return !BraveOriginStartupView::IsShowing();
  }));

  // A new browser window should have been created by Start_ChromiumImpl.
  EXPECT_GT(chrome::GetTotalBrowserCount(), browser_count_before);
}

// Verifies that StartupBrowserCreator::Start() proceeds normally (no dialog)
// when ShouldShowDialog is false.
IN_PROC_BROWSER_TEST_F(BraveOriginStartupViewBrowserTest,
                       StartOverrideSkipsDialogWhenNotNeeded) {
  BraveOriginStartupView::SetShouldShowDialogForTesting(false);

  size_t browser_count_before = chrome::GetTotalBrowserCount();

  StartupBrowserCreator browser_creator;
  StartupProfileInfo profile_info{browser()->profile(),
                                  StartupProfileMode::kBrowserWindow};
  bool result = browser_creator.Start(*base::CommandLine::ForCurrentProcess(),
                                      base::FilePath(), profile_info,
                                      {browser()->profile()});
  EXPECT_TRUE(result);
  EXPECT_FALSE(BraveOriginStartupView::IsShowing());

  // Start_ChromiumImpl should have run directly, creating a new browser.
  EXPECT_GT(chrome::GetTotalBrowserCount(), browser_count_before);
}
