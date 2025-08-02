// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/ui/ai_chat/ai_chat_agent_profile_helper.h"
#include "brave/browser/ui/webui/ai_chat/ai_chat_ui.h"
#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/constants/brave_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profile_test_util.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/profiles/profile_picker.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/side_panel/side_panel_coordinator.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

class AIChatProfileBrowserTest : public InProcessBrowserTest {
 public:
  AIChatProfileBrowserTest() {
    scoped_feature_list_.InitAndEnableFeature(
        ai_chat::features::kAIChatAgenticProfile);
  }
  AIChatProfileBrowserTest(const AIChatProfileBrowserTest&) = delete;
  AIChatProfileBrowserTest& operator=(const AIChatProfileBrowserTest&) = delete;

 protected:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    // Browser should never launch with the AI Chat profile
    ASSERT_FALSE(browser()->profile()->IsAIChatAgent());
  }

  void VerifyAIChatSidePanelShowing(Browser* browser) {
    auto* side_panel_coordinator =
        browser->GetFeatures().side_panel_coordinator();
    ASSERT_TRUE(side_panel_coordinator);
    auto* side_panel_web_contents =
        side_panel_coordinator->GetWebContentsForTest(
            SidePanelEntry::Id::kChatUI);
    ASSERT_TRUE(side_panel_web_contents);
    auto* web_ui = side_panel_web_contents->GetWebUI();
    ASSERT_TRUE(web_ui);
    auto* ai_chat_ui = web_ui->GetController()->GetAs<AIChatUI>();
    ASSERT_TRUE(ai_chat_ui);

    content::WaitForLoadStop(side_panel_web_contents);
  }

  Browser* CallOpenBrowserWindowForAiChatAgentProfile(Profile* from_profile) {
    base::RunLoop run_loop;
    Browser* browser = nullptr;
    OpenBrowserWindowForAIChatAgentProfile(
        from_profile,
        base::BindLambdaForTesting([&](Browser* ai_profile_browser) {
          browser = ai_profile_browser;
          run_loop.Quit();
        }));
    run_loop.Run();
    return browser;
  }

  Browser* FindAIChatBrowser() {
    for (Browser* browser : *BrowserList::GetInstance()) {
      if (browser->profile()->IsAIChatAgent()) {
        return browser;
      }
    }
    return nullptr;
  }

  base::test::ScopedFeatureList scoped_feature_list_;
};

// Test that OpenBrowserWindowForAIChatAgentProfile creates a browser window
IN_PROC_BROWSER_TEST_F(AIChatProfileBrowserTest,
                       OpenBrowserWindowForAIChatAgentProfile) {
  // Keep track of initial browser count
  EXPECT_EQ(1u, chrome::GetTotalBrowserCount());

  // First request to open AI Chat Agent Profile browser window
  // should be a noop because this profile is not opted in to AI Chat.
  Browser* opened_browser =
      CallOpenBrowserWindowForAiChatAgentProfile(GetProfile());
  EXPECT_EQ(nullptr, opened_browser);
  EXPECT_EQ(1u, chrome::GetTotalBrowserCount());
  EXPECT_FALSE(GetProfile()->IsAIChatAgent());

  SetUserOptedIn(GetProfile()->GetPrefs(), true);

  // Second request to open AI Chat Agent Profile browser window
  // should open a new browser window
  opened_browser = CallOpenBrowserWindowForAiChatAgentProfile(GetProfile());

  // Verify that a new browser window was opened
  EXPECT_EQ(2u, chrome::GetTotalBrowserCount());

  // Find the AI Chat browser
  Browser* ai_chat_browser = FindAIChatBrowser();
  ASSERT_TRUE(ai_chat_browser);
  EXPECT_EQ(opened_browser, ai_chat_browser);

  // Verify the profile is reported as the AI Chat profile, although it is
  // already used in FindAIChatBrowser - that could change and we want to make
  // sure IsAIChatContentAgentProfile is explicitly tested.
  EXPECT_TRUE(ai_chat_browser->profile()->IsAIChatAgent());
  // Verify the profile path matches the AI Chat profile path
  EXPECT_TRUE(ai_chat_browser->profile()->GetPath().BaseName().value() ==
              brave::kAIChatAgentProfileDir);

  // Verify the AI Chat browser has the side panel opened to Chat UI
  VerifyAIChatSidePanelShowing(ai_chat_browser);
}

// Test that multiple calls to OpenBrowserWindowForAIChatAgentProfile work
// correctly
IN_PROC_BROWSER_TEST_F(AIChatProfileBrowserTest,
                       OpenBrowserWindowForAIChatAgentProfile_MultipleOpens) {
  SetUserOptedIn(GetProfile()->GetPrefs(), true);

  EXPECT_EQ(1u, chrome::GetTotalBrowserCount());

  // First call to open AI Chat profile
  Browser* opened_browser =
      CallOpenBrowserWindowForAiChatAgentProfile(GetProfile());
  ASSERT_TRUE(opened_browser);
  EXPECT_EQ(2u, chrome::GetTotalBrowserCount());

  EXPECT_EQ(opened_browser, FindAIChatBrowser());

  // Second call should not open a new one
  Browser* second_opened_browser =
      CallOpenBrowserWindowForAiChatAgentProfile(GetProfile());
  ASSERT_TRUE(second_opened_browser);
  EXPECT_EQ(opened_browser, second_opened_browser);
  EXPECT_EQ(2u, chrome::GetTotalBrowserCount());

  VerifyAIChatSidePanelShowing(opened_browser);

  // Close browser
  CloseBrowserSynchronously(opened_browser);
  EXPECT_EQ(1u, chrome::GetTotalBrowserCount());

  // Subsequent call to open should open a new browser
  Browser* third_opened_browser =
      CallOpenBrowserWindowForAiChatAgentProfile(GetProfile());
  ASSERT_TRUE(third_opened_browser);
  EXPECT_EQ(2u, chrome::GetTotalBrowserCount());
  EXPECT_EQ(third_opened_browser, FindAIChatBrowser());

  VerifyAIChatSidePanelShowing(third_opened_browser);
}

// // Tests for AI Chat Agent Profile startup behavior
class AIChatProfileStartupBrowserTest : public AIChatProfileBrowserTest {
 public:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpCommandLine(command_line);
    // Make sure InProcessBrowserTest does not add about:blank as a startup URL.
    set_open_about_blank_on_browser_launch(false);
    set_exit_when_last_browser_closes(true);
  }
};

IN_PROC_BROWSER_TEST_F(AIChatProfileStartupBrowserTest,
                       PRE_AIChatProfileDoesNotAffectStartup) {
  // Create AI Chat Agent profile and browser window
  SetUserOptedIn(GetProfile()->GetPrefs(), true);
  Browser* opened_browser =
      CallOpenBrowserWindowForAiChatAgentProfile(GetProfile());
  ASSERT_TRUE(opened_browser);
  // Verify that a new browser window was opened
  EXPECT_EQ(2u, chrome::GetTotalBrowserCount());

  // Need to close the browser window manually so that the real test does not
  // treat it as session restore.
  CloseAllBrowsers();
}

IN_PROC_BROWSER_TEST_F(AIChatProfileStartupBrowserTest,
                       AIChatProfileDoesNotAffectStartup) {
  // Verify that on restart, the profile picker is not shown and the original
  // profile is used. This tests the override in profile_picker.cc.
  EXPECT_FALSE(ProfilePicker::IsOpen());
  // If the profile picker is open then there are no browser open,
  // so make sure we have a default browser open.
  EXPECT_EQ(1u, chrome::GetTotalBrowserCount());
  EXPECT_EQ(nullptr, FindAIChatBrowser());
}

IN_PROC_BROWSER_TEST_F(AIChatProfileStartupBrowserTest,
                       PRE_ProfileNotReopenedOnStartup) {
  // Quit the first session with main profile and AI Chat profile
  // still open.
  SetUserOptedIn(GetProfile()->GetPrefs(), true);
  Browser* opened_browser =
      CallOpenBrowserWindowForAiChatAgentProfile(GetProfile());
  ASSERT_TRUE(opened_browser);
  EXPECT_EQ(2u, chrome::GetTotalBrowserCount());
  EXPECT_EQ(opened_browser, FindAIChatBrowser());
  // Leave the browser windows open
}

IN_PROC_BROWSER_TEST_F(AIChatProfileStartupBrowserTest,
                       ProfileNotReopenedOnStartup) {
  // Verify the AI Chat profile is not opened on startup
  // This tests the override in startup_browser_creator.cc.
  EXPECT_EQ(1u, chrome::GetTotalBrowserCount());
  EXPECT_EQ(nullptr, FindAIChatBrowser());
}

}  // namespace ai_chat
