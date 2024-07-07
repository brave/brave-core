/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/command_line.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/brave_browser_features.h"
#include "brave/components/brave_rewards/content/common/pref_names.h"
#include "chrome/browser/first_run/first_run.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"
#include "testing/gtest/include/gtest/gtest.h"

// Check DayZeroBrowserUIExptManager is initialized properly.
// This test will catch first run sentinel creation time fetching timing
// changes.
class DayZeroBrowserUIExptBrowserTest
    : public InProcessBrowserTest,
      public testing::WithParamInterface<bool> {
 public:
  DayZeroBrowserUIExptBrowserTest() {
    if (IsDayZeroEnabled()) {
      feature_list_.InitAndEnableFeature(features::kBraveDayZeroExperiment);
    }
  }

  ~DayZeroBrowserUIExptBrowserTest() override = default;

  void SetUpCommandLine(base::CommandLine* command_line) override {
    // In browser test, first run sentinel file is not created w/o this switch.
    command_line->AppendSwitch(switches::kForceFirstRun);
  }

  bool IsDayZeroEnabled() { return GetParam(); }

  base::test::ScopedFeatureList feature_list_;
};

IN_PROC_BROWSER_TEST_P(DayZeroBrowserUIExptBrowserTest, InitTest) {
  auto* prefs = browser()->profile()->GetPrefs();

  // Button is hidden by default when expt feature is enabled.
  const bool button_is_hidden =
      !prefs->GetBoolean(brave_rewards::prefs::kShowLocationBarButton);
  EXPECT_EQ(IsDayZeroEnabled(), button_is_hidden);
}

INSTANTIATE_TEST_SUITE_P(DayZeroExpt,
                         DayZeroBrowserUIExptBrowserTest,
                         testing::Bool());
