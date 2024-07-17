/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/session_crashed_bubble_view.h"

#include "brave/browser/brave_local_state_prefs.h"
#include "brave/browser/metrics/pref_names.h"
#include "chrome/browser/prefs/browser_prefs.h"
#include "chrome/browser/ui/cocoa/confirm_quit.h"
#include "chrome/test/base/scoped_testing_local_state.h"
#include "chrome/test/base/testing_browser_process.h"
#include "components/metrics/metrics_pref_names.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

class SessionCrashedBubbleViewTest : public testing::Test {
 public:
  SessionCrashedBubbleViewTest() = default;
  ~SessionCrashedBubbleViewTest() override = default;

  void SetUp() override {
    RegisterLocalState(local_state_.registry());
    TestingBrowserProcess::GetGlobal()->SetLocalState(&local_state_);
  }
  void TearDown() override {
    TestingBrowserProcess::GetGlobal()->SetLocalState(nullptr);
  }
  TestingPrefServiceSimple local_state_;
};

TEST_F(SessionCrashedBubbleViewTest, CrashReportPermissionAskDialog) {
  local_state_.SetBoolean(metrics::prefs::kMetricsReportingEnabled, false);
  EXPECT_TRUE(
      SessionCrashedBubbleView::ShouldShowCrashReportPermissionAskDialog());

  local_state_.SetBoolean(metrics::prefs::kMetricsReportingEnabled, true);
  EXPECT_FALSE(
      SessionCrashedBubbleView::ShouldShowCrashReportPermissionAskDialog());
}

TEST_F(SessionCrashedBubbleViewTest,
       CrashReportPermissionAskDialogPolicyManaged) {
  EXPECT_TRUE(
      SessionCrashedBubbleView::ShouldShowCrashReportPermissionAskDialog());
  local_state_.SetManagedPref(metrics::prefs::kDontAskForCrashReporting,
                              base::Value(true));
  EXPECT_FALSE(
      SessionCrashedBubbleView::ShouldShowCrashReportPermissionAskDialog());
}

TEST_F(SessionCrashedBubbleViewTest,
       CrashReportPermissionAskDialogPolicyDontAskPref) {
  local_state_.SetBoolean(metrics::prefs::kMetricsReportingEnabled, true);
  EXPECT_FALSE(
      SessionCrashedBubbleView::ShouldShowCrashReportPermissionAskDialog());
}
