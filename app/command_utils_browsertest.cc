// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/app/command_utils.h"

#include "base/containers/contains.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/commands/common/features.h"
#include "build/buildflag.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "testing/gtest/include/gtest/gtest.h"

#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_LINUX)
#include "base/test/mock_callback.h"
#include "chrome/browser/first_run/scoped_relaunch_chrome_browser_override.h"
#include "chrome/browser/first_run/upgrade_util.h"
#endif  // BUILDFLAG(IS_WIN) || BUILDFLAG(IS_LINUX)

class CommandUtilsBrowserTest : public InProcessBrowserTest {
 public:
  CommandUtilsBrowserTest() {
#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_LINUX)
    // Expect a browser relaunch late in browser shutdown.
    mock_relaunch_callback_ = std::make_unique<::testing::StrictMock<
        base::MockCallback<upgrade_util::RelaunchChromeBrowserCallback>>>();
    EXPECT_CALL(*mock_relaunch_callback_, Run);
    relaunch_chrome_override_ =
        std::make_unique<upgrade_util::ScopedRelaunchChromeBrowserOverride>(
            mock_relaunch_callback_->Get());
#endif  // BUILDFLAG(IS_WIN) || BUILDFLAG(IS_LINUX)
  }
  ~CommandUtilsBrowserTest() override = default;

  void SetUp() override {
    features_.InitAndEnableFeature(commands::features::kBraveCommands);
    InProcessBrowserTest::SetUp();
  }

 private:
  base::test::ScopedFeatureList features_;
#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_LINUX)
  std::unique_ptr<
      base::MockCallback<upgrade_util::RelaunchChromeBrowserCallback>>
      mock_relaunch_callback_;
  std::unique_ptr<upgrade_util::ScopedRelaunchChromeBrowserOverride>
      relaunch_chrome_override_;
#endif  // BUILDFLAG(IS_WIN) || BUILDFLAG(IS_LINUX)
};

// This test is currently flaky on all Desktop platforms. On Windows it
// occasionally crashes, on Linux it fails an expectation in pref observer, and
// on MacOS it times out. Disabling on all platforms until further
// investigation can be done.
// This test is a sanity check - if commands fail here but work when testing
// things manually there's probably a conflict with some of the other commands,
// in which case we can just add it to the ignored commands list.
IN_PROC_BROWSER_TEST_F(CommandUtilsBrowserTest,
                       DISABLED_AllCommandsShouldBeExecutableWithoutCrash) {
  // Some commands, particularly those that create dialogs introduce some test
  // flakes, so we disable them.
  constexpr int kKnownGoodCommandsThatSometimesBreakTest[] = {
      IDC_PRINT,
      IDC_BASIC_PRINT,
      IDC_OPEN_FILE,
      IDC_SAVE_PAGE,
      IDC_SHOW_AVATAR_MENU,
      IDC_SHOW_MANAGEMENT_PAGE,
#if BUILDFLAG(IS_MAC)
      IDC_FOCUS_THIS_TAB,
      IDC_FOCUS_TOOLBAR,
      IDC_FOCUS_LOCATION,
      IDC_FOCUS_SEARCH,
      IDC_FOCUS_MENU_BAR,
      IDC_FOCUS_NEXT_PANE,
      IDC_FOCUS_PREVIOUS_PANE,
      IDC_FOCUS_BOOKMARKS,
      IDC_FOCUS_INACTIVE_POPUP_FOR_ACCESSIBILITY,
      IDC_FOCUS_WEB_CONTENTS_PANE,
      IDC_TOGGLE_FULLSCREEN_TOOLBAR,
      IDC_CONTENT_CONTEXT_EXIT_FULLSCREEN,
      IDC_FULLSCREEN,
      IDC_TOGGLE_VERTICAL_TABS,
      IDC_TOGGLE_VERTICAL_TABS_WINDOW_TITLE,
#endif

      IDC_EXIT};

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), GURL("chrome://newtab")));
  const auto& commands = commands::GetCommands();
  for (const auto& command : commands) {
    if (base::Contains(kKnownGoodCommandsThatSometimesBreakTest, command)) {
      continue;
    }

#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_LINUX)
    if (command == IDC_CLOSE_TAB) {
      // If this is the only tab the browser will exit, so add a new tab
      // before executing the command
      SCOPED_TRACE(testing::Message() << commands::GetCommandName(command));
      chrome::ExecuteCommand(browser(), IDC_NEW_TAB);
      base::RunLoop().RunUntilIdle();
    } else if (command == IDC_CLOSE_WINDOW) {
      // If this is the only window the browser will exit, so add a new window
      // before executing the command
      SCOPED_TRACE(testing::Message() << commands::GetCommandName(command));
      chrome::ExecuteCommand(browser(), IDC_NEW_WINDOW);
      base::RunLoop().RunUntilIdle();
    }
#endif  // BUILDFLAG(IS_WIN) || BUILDFLAG(IS_LINUX)

    // Use the first browser instance for each command.
    SelectFirstBrowser();

    SCOPED_TRACE(testing::Message() << commands::GetCommandName(command));
    LOG(INFO) << command << ": " << commands::GetCommandName(command);
    chrome::ExecuteCommand(browser(), command);
  }
  SCOPED_TRACE(testing::Message() << commands::GetCommandName(IDC_EXIT));
  chrome::ExecuteCommand(browser(), IDC_EXIT);
}
