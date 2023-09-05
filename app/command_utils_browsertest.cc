// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "base/containers/contains.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/scoped_feature_list.h"
#include "brave/app/command_utils.h"
#include "brave/components/commands/common/features.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "testing/gtest/include/gtest/gtest.h"

class CommandUtilsBrowserTest : public InProcessBrowserTest,
                                public testing::WithParamInterface<int> {
 public:
  CommandUtilsBrowserTest() = default;
  ~CommandUtilsBrowserTest() override = default;

  void SetUp() override {
    features_.InitAndEnableFeature(commands::features::kBraveCommands);
    InProcessBrowserTest::SetUp();
  }

  static std::vector<int> GetCommands() {
    // Some commands, particularly those that create dialogs introduce some test
    // flakes, so we disable them.
    constexpr int kKnownGoodCommandsThatSometimesBreakTest[] = {
      IDC_PRINT,
      IDC_BASIC_PRINT,
      IDC_OPEN_FILE,
      IDC_SAVE_PAGE,
      IDC_SHOW_MANAGEMENT_PAGE,
      IDC_ADD_NEW_PROFILE,
#if BUILDFLAG(IS_MAC)
      IDC_FULLSCREEN,
#endif

      IDC_EXIT,
    };

    std::vector<int> filtered_commands;
    base::ranges::copy_if(
        commands::GetCommands(), std::back_inserter(filtered_commands),
        [&](const int command_id) {
          return !base::Contains(kKnownGoodCommandsThatSometimesBreakTest,
                                 command_id);
        });
    return filtered_commands;
  }

 private:
  base::test::ScopedFeatureList features_;
};

// This test is a sanity check - if commands fail here but work when testing
// things manually there's probably a conflict with some of the other commands,
// in which case we can just add it to the ignored commands list.
IN_PROC_BROWSER_TEST_P(CommandUtilsBrowserTest,
                       AllCommandsShouldBeExecutableWithoutCrash) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), GURL("chrome://newtab")));
  const int command = GetParam();
  LOG(INFO) << "Running command " << commands::GetCommandName(command);
  chrome::ExecuteCommand(browser(), command);
}

INSTANTIATE_TEST_SUITE_P(
    ,
    CommandUtilsBrowserTest,
    testing::ValuesIn(CommandUtilsBrowserTest::GetCommands()),
    [](const testing::TestParamInfo<int>& p) {
      return base::NumberToString(p.param);
    });
