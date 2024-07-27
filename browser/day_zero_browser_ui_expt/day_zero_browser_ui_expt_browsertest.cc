/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <variant>

#include "base/command_line.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/brave_browser_features.h"
#include "brave/browser/day_zero_browser_ui_expt/pref_names.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/first_run/first_run.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"
#include "testing/gtest/include/gtest/gtest.h"

class DayZeroBrowserUIExptBrowserTest
    : public InProcessBrowserTest,
      public ::testing::WithParamInterface<std::tuple<bool, bool>> {
 public:
  DayZeroBrowserUIExptBrowserTest() {
    if (IsDayZeroEnabled()) {
      feature_list_.InitAndEnableFeatureWithParameters(
          features::kBraveDayZeroExperiment,
          {{"variant", "a"}});
    }
  }

  ~DayZeroBrowserUIExptBrowserTest() override = default;

  void SetUpCommandLine(base::CommandLine* command_line) override {
    // In browser test, first run sentinel file is not created w/o this switch.
    command_line->AppendSwitch(IsFirstRun() ? switches::kForceFirstRun
                                            : switches::kNoFirstRun);
  }

  bool IsDayZeroEnabled() { return std::get<0>(GetParam()); }
  bool IsFirstRun() { return std::get<1>(GetParam()); }

  base::test::ScopedFeatureList feature_list_;
};

IN_PROC_BROWSER_TEST_P(DayZeroBrowserUIExptBrowserTest, InitTest) {
  auto* prefs = browser()->profile()->GetPrefs();

  const bool button_is_hidden =
      !prefs->GetBoolean(brave_rewards::prefs::kShowLocationBarButton);

  // We don't apply day-zero experiment to existing users.
  if (!IsFirstRun()) {
    EXPECT_FALSE(button_is_hidden);
    return;
  }

  // Button is hidden by default when expt feature is enabled.
  EXPECT_EQ(IsDayZeroEnabled(), button_is_hidden);
}

INSTANTIATE_TEST_SUITE_P(DayZeroExpt,
                         DayZeroBrowserUIExptBrowserTest,
                         testing::Combine(testing::Bool(), testing::Bool()));

class DayZeroBrowserUIExptSecondLaunchBrowserTest
    : public InProcessBrowserTest {
 public:
  DayZeroBrowserUIExptSecondLaunchBrowserTest() {
    feature_list_.InitAndEnableFeatureWithParameters(
        features::kBraveDayZeroExperiment, {{"variant", "a"}});
  }

  ~DayZeroBrowserUIExptSecondLaunchBrowserTest() override = default;

  base::test::ScopedFeatureList feature_list_;
};

IN_PROC_BROWSER_TEST_F(DayZeroBrowserUIExptSecondLaunchBrowserTest,
                       PRE_SecondLaunch) {
  // To simulate this is first run and day-zero experiment is applied.
  g_browser_process->local_state()->SetBoolean(kDayZeroExperimentTargetInstall,
                                               true);
}

// Check day-zero experiment is stiil applied at non first run if it's applied
// at first run.
IN_PROC_BROWSER_TEST_F(DayZeroBrowserUIExptSecondLaunchBrowserTest,
                       SecondLaunch) {
  auto* prefs = browser()->profile()->GetPrefs();
  const bool button_is_hidden =
      !prefs->GetBoolean(brave_rewards::prefs::kShowLocationBarButton);
  EXPECT_TRUE(button_is_hidden);
}
