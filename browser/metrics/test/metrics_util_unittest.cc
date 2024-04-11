/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/metrics/metrics_util.h"
#include "brave/browser/brave_local_state_prefs.h"
#include "brave/browser/metrics/pref_names.h"
#include "chrome/browser/prefs/browser_prefs.h"
#include "chrome/browser/ui/cocoa/confirm_quit.h"
#include "chrome/test/base/scoped_testing_local_state.h"
#include "chrome/test/base/testing_browser_process.h"
#include "components/metrics/metrics_pref_names.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

class MetricsUtilTest : public testing::Test {
 public:
  MetricsUtilTest() = default;
  ~MetricsUtilTest() override = default;

  void SetUp() override {
    RegisterLocalState(local_state_.registry());
    TestingBrowserProcess::GetGlobal()->SetLocalState(&local_state_);
  }
  void TearDown() override {
    TestingBrowserProcess::GetGlobal()->SetLocalState(nullptr);
  }
  TestingPrefServiceSimple local_state_;
};

TEST_F(MetricsUtilTest, CrashReportPermissionAskDialog) {
  local_state_.SetBoolean(metrics::prefs::kMetricsReportingEnabled, false);
  EXPECT_TRUE(metrics::ShouldShowCrashReportPermissionAskDialog());

  local_state_.SetBoolean(metrics::prefs::kMetricsReportingEnabled, true);
  EXPECT_FALSE(metrics::ShouldShowCrashReportPermissionAskDialog());
}

TEST_F(MetricsUtilTest, CrashReportPermissionAskDialogPolicyManaged) {
  EXPECT_TRUE(metrics::ShouldShowCrashReportPermissionAskDialog());
  local_state_.SetManagedPref(metrics::prefs::kDontAskForCrashReporting,
                              base::Value(true));
  EXPECT_FALSE(metrics::ShouldShowCrashReportPermissionAskDialog());
}

TEST_F(MetricsUtilTest, CrashReportPermissionAskDialogPolicyDontAskPref) {
  local_state_.SetBoolean(metrics::prefs::kMetricsReportingEnabled, true);
  EXPECT_FALSE(metrics::ShouldShowCrashReportPermissionAskDialog());
}
