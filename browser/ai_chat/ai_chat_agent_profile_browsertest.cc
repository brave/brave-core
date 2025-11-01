// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "base/containers/contains.h"
#include "base/containers/fixed_flat_set.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/ai_chat/ai_chat_agent_profile_helper.h"
#include "brave/browser/ai_chat/ai_chat_service_factory.h"
#include "brave/browser/ui/webui/ai_chat/ai_chat_ui.h"
#include "brave/components/ai_chat/core/browser/ai_chat_service.h"
#include "brave/components/ai_chat/core/browser/conversation_handler.h"
#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/constants/brave_constants.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profile_test_util.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/profiles/profile_picker.h"
#include "chrome/browser/ui/profiles/profile_view_utils.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/side_panel/side_panel_coordinator.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/webui_url_constants.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

class AIChatAgentProfileBrowserTest : public InProcessBrowserTest {
 public:
  explicit AIChatAgentProfileBrowserTest(bool enable_feature = true) {
    if (enable_feature) {
      scoped_feature_list_.InitAndEnableFeature(
          ai_chat::features::kAIChatAgentProfile);
    } else {
      scoped_feature_list_.InitAndDisableFeature(
          ai_chat::features::kAIChatAgentProfile);
    }
  }
  AIChatAgentProfileBrowserTest(const AIChatAgentProfileBrowserTest&) = delete;
  AIChatAgentProfileBrowserTest& operator=(
      const AIChatAgentProfileBrowserTest&) = delete;

 protected:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    // Browser should never launch with the AI Chat profile
    if (browser()) {
      ASSERT_FALSE(browser()->profile()->IsAIChatAgent());
    }
  }

  void VerifyAIChatSidePanelShowing(Browser* browser,
                                    bool should_open_panel = false) {
    auto* side_panel_coordinator =
        browser->GetFeatures().side_panel_coordinator();
    ASSERT_TRUE(side_panel_coordinator);
    if (should_open_panel) {
      side_panel_coordinator->Show(SidePanelEntry::Id::kChatUI);
    }
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
    OpenBrowserWindowForAIChatAgentProfileForTesting(
        *from_profile,
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

  Profile* GetProfile() { return browser()->profile(); }

  base::test::ScopedFeatureList scoped_feature_list_;
};

// Test that OpenBrowserWindowForAIChatAgentProfile creates a browser window
IN_PROC_BROWSER_TEST_F(AIChatAgentProfileBrowserTest,
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
  // Verify the built-in profile title is set as the local user name
  ProfileAttributesEntry* profile_attributes =
      GetProfileAttributesFromProfile(ai_chat_browser->profile());
  EXPECT_EQ(u"Leo AI Content Agent", profile_attributes->GetLocalProfileName());

  // Verify the AI Chat browser has the side panel opened to Chat UI
  VerifyAIChatSidePanelShowing(ai_chat_browser);

  // Verify the new tab page is the AI Chat Agent new tab page
  auto* ntp_rfh = ui_test_utils::NavigateToURL(
      ai_chat_browser, GURL(chrome::kChromeUINewTabURL));
  ASSERT_TRUE(ntp_rfh);
  EXPECT_TRUE(content::EvalJs(ntp_rfh,
                              "!!document.querySelector(`html[data-test-id="
                              "'brave-ai-chat-agent-new-tab-page']`)")
                  .ExtractBool());

  // Verify content agent tools are available in the agent profile
  auto* agent_ai_chat_service =
      AIChatServiceFactory::GetForBrowserContext(ai_chat_browser->profile());
  ASSERT_NE(agent_ai_chat_service, nullptr);
  auto* agent_conversation = agent_ai_chat_service->CreateConversation();

  // This is a little clunky - if the content agent tool provider is not
  // the first provider then this will need to become more advanced than
  // GetFirstToolProviderForTesting.
  auto* tool_provider = agent_conversation->GetFirstToolProviderForTesting();
  ASSERT_NE(tool_provider, nullptr);

  // Expected tool names in the content agent profile
  constexpr auto kContentAgentToolNames =
      base::MakeFixedFlatSet<std::string_view>(
          {"click_element", "drag_and_release", "navigate_history",
           "move_mouse", mojom::kNavigateToolName, "scroll_element",
           "select_dropdown", "type_text", "wait"});

  // Verify all tools match expected names
  for (const auto& tool : tool_provider->GetTools()) {
    EXPECT_TRUE(base::Contains(kContentAgentToolNames, tool->Name()))
        << "Tool " << tool->Name() << " should be in the agent profile";
  }

  // Verify the tools aren't available in the regular profile
  auto* regular_ai_chat_service =
      AIChatServiceFactory::GetForBrowserContext(GetProfile());
  ASSERT_NE(regular_ai_chat_service, nullptr);
  auto* regular_conversation = regular_ai_chat_service->CreateConversation();
  ASSERT_NE(regular_conversation, nullptr);
  auto* regular_tool_provider =
      regular_conversation->GetFirstToolProviderForTesting();
  // Regular profile might have no other tool provider, but if it does then
  // check it isn't the ContentAgentToolProvider.
  if (regular_tool_provider) {
    for (const auto& tool : regular_tool_provider->GetTools()) {
      EXPECT_FALSE(base::Contains(kContentAgentToolNames, tool->Name()))
          << "Tool " << tool->Name() << " should not be in the regular profile";
    }
  }
}

// Test that multiple calls to OpenBrowserWindowForAIChatAgentProfile work
// correctly
IN_PROC_BROWSER_TEST_F(AIChatAgentProfileBrowserTest,
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

// UI Tests for AI Chat Agent Profile features
// TODO(https://github.com/brave/brave-browser/issues/48165): This should be
// converted to an interactive_uitest.

class AIChatAgentProfileWebUIContentBrowserTest
    : public AIChatAgentProfileBrowserTest,
      public testing::WithParamInterface<bool> {
 public:
  AIChatAgentProfileWebUIContentBrowserTest()
      : AIChatAgentProfileBrowserTest(GetParam()) {}

  void SetUp() override { AIChatAgentProfileBrowserTest::SetUp(); }

 protected:
  void WaitForElementInSidePanel(Browser* browser,
                                 const std::string& selector) {
    // TODO(https://github.com/brave/brave-browser/issues/48165): This would be
    // nicer in an interactive_uitest.
    auto* side_panel_web_contents =
        browser->GetFeatures().side_panel_coordinator()->GetWebContentsForTest(
            SidePanelEntry::Id::kChatUI);
    constexpr char kWaitForAIChatRenderScript[] = R"(
      new Promise((resolve, reject) => {
        const TIMEOUT_SECONDS = 10;

        let element = document.querySelector($1);
        if (element) {
          resolve(true);
          return;
        }

        const timerID = window.setTimeout(() => {
          observer.disconnect();
          let element = document.querySelector($1);
          if (element) {
            resolve(true);
          } else {
            reject(new Error("Timed out waiting for '" + $1 + "'."));
          }
        }, TIMEOUT_SECONDS * 1000);

        const observer = new MutationObserver(() => {
          let element = document.querySelector($1);
          if (element) {
            clearTimeout(timerID);
            observer.disconnect();
            resolve(true);
          }
        });
        observer.observe(document.documentElement,
            { childList: true, subtree: true });
      });
    )";

    auto result = content::EvalJs(
        side_panel_web_contents,
        content::JsReplace(kWaitForAIChatRenderScript, selector));
    ASSERT_TRUE(result.ExtractBool());
  }

  bool IsElementInSidePanel(Browser* browser, const std::string& selector) {
    auto* side_panel_web_contents =
        browser->GetFeatures().side_panel_coordinator()->GetWebContentsForTest(
            SidePanelEntry::Id::kChatUI);
    auto result = content::EvalJs(
        side_panel_web_contents,
        content::JsReplace("!!document.querySelector($1)", selector));
    return result.ExtractBool();
  }

  void WaitForAIChatRender(Browser* browser) {
    // Wait for initial data to be received and full UI to be rendered.
    WaitForElementInSidePanel(browser, "[data-testid=\"main\"]");
  }

  bool IsAIChatAgentProfileTooltipPresent(Browser* browser) {
    return IsElementInSidePanel(browser,
                                "[data-testid=\"agent-profile-tooltip\"]");
  }

  bool IsAIChatAgentProfileLaunchButtonPresent(Browser* browser) {
    return IsElementInSidePanel(browser,
                                "[title=\"Open Leo AI Content Agent Window\"]");
  }

  void WaitForAIChatAgentProfileLaunchButton(Browser* browser) {
    WaitForElementInSidePanel(browser,
                              "[title=\"Open Leo AI Content Agent Window\"]");
  }
};

IN_PROC_BROWSER_TEST_P(AIChatAgentProfileWebUIContentBrowserTest,
                       AgentProfileElements) {
  bool feature_enabled = GetParam();
  EXPECT_EQ(1u, chrome::GetTotalBrowserCount());
  EXPECT_FALSE(GetProfile()->IsAIChatAgent());

  VerifyAIChatSidePanelShowing(browser(), true);
  WaitForAIChatRender(browser());

  if (!feature_enabled) {
    // When the feature is disabled, no buttons are shown
    EXPECT_FALSE(IsAIChatAgentProfileLaunchButtonPresent(browser()));
    EXPECT_FALSE(IsAIChatAgentProfileTooltipPresent(browser()));
    return;
  }

  // When not opted in, no agent profile button is shown
  EXPECT_FALSE(IsAIChatAgentProfileTooltipPresent(browser()));
  EXPECT_FALSE(IsAIChatAgentProfileLaunchButtonPresent(browser()));

  // When opted in, the agent profile button is shown
  SetUserOptedIn(GetProfile()->GetPrefs(), true);
  WaitForAIChatAgentProfileLaunchButton(browser());
  EXPECT_FALSE(IsAIChatAgentProfileTooltipPresent(browser()));

  // In the AI Chat agent profile, the tooltip is shown but
  // not the launch button.
  Browser* opened_browser =
      CallOpenBrowserWindowForAiChatAgentProfile(GetProfile());
  VerifyAIChatSidePanelShowing(opened_browser);
  WaitForAIChatRender(opened_browser);
  ASSERT_TRUE(opened_browser);
  EXPECT_FALSE(IsAIChatAgentProfileLaunchButtonPresent(opened_browser));
  EXPECT_TRUE(IsAIChatAgentProfileTooltipPresent(opened_browser));
}

INSTANTIATE_TEST_SUITE_P(,
                         AIChatAgentProfileWebUIContentBrowserTest,
                         testing::Values(true, false),
                         [](const testing::TestParamInfo<bool>& info) {
                           return info.param ? "AIChatAgentProfileEnabled"
                                             : "AIChatAgentProfileDisabled";
                         });

// // Tests for AI Chat Agent Profile startup behavior
class AIChatAgentProfileStartupBrowserTest
    : public AIChatAgentProfileBrowserTest {
 public:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpCommandLine(command_line);
    // Avoid providing a URL for the browser to open, allows the profile picker
    // to be displayed on startup when it is enabled.
    set_open_about_blank_on_browser_launch(false);
  }
};

IN_PROC_BROWSER_TEST_F(AIChatAgentProfileStartupBrowserTest,
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

IN_PROC_BROWSER_TEST_F(AIChatAgentProfileStartupBrowserTest,
                       AIChatProfileDoesNotAffectStartup) {
  // Verify that on restart, the profile picker is not shown and the original
  // profile is used. This tests the override in profile_picker.cc.
  EXPECT_FALSE(ProfilePicker::IsOpen());
  // If the profile picker is open then there are no browser open,
  // so make sure we have a default browser open.
  EXPECT_EQ(1u, chrome::GetTotalBrowserCount());
  EXPECT_EQ(nullptr, FindAIChatBrowser());
}

IN_PROC_BROWSER_TEST_F(AIChatAgentProfileStartupBrowserTest,
                       PRE_AIChatProfileDoesNotAffectStartup_MultiplePrevious) {
  // If we previously showed the profile picker because
  // the user had multiple profiles but now only has one (aside from AI Chat
  // agent profile), the profile picker should not be shown. Without modifying
  // ProfileManager::GetNumberOfProfiles, ProfilePicker::GetStartupModeReason
  // would decide to show the picker because the number of profiles is > 1 and
  // we have shown the profile picker before.

  // Create AI Chat Agent profile and browser window
  SetUserOptedIn(GetProfile()->GetPrefs(), true);
  Browser* opened_browser =
      CallOpenBrowserWindowForAiChatAgentProfile(GetProfile());
  ASSERT_TRUE(opened_browser);
  // Verify that a new browser window was opened
  EXPECT_EQ(2u, chrome::GetTotalBrowserCount());

  // Simulate the profile picker having been shown without the user
  // unchecking the "Show profile picker on startup" checkbox.
  // This tests where the user previously had multiple profiles but
  // now only has one regular profile and one AI Chat agent profile.
  // Since they would have seen the profile picker before, this pref
  // will be true.
  g_browser_process->local_state()->SetBoolean(
      prefs::kBrowserProfilePickerShown, true);

  // Need to close the browser window manually so that the real test does not
  // treat it as session restore.
  CloseAllBrowsers();
}

IN_PROC_BROWSER_TEST_F(AIChatAgentProfileStartupBrowserTest,
                       AIChatProfileDoesNotAffectStartup_MultiplePrevious) {
  // Verify that on restart, the profile picker is not shown and the original
  // profile is used. This tests the override in profile_picker.cc.
  EXPECT_FALSE(ProfilePicker::IsOpen());
  // If the profile picker is open then there are no browser open,
  // so make sure we have a default browser open.
  EXPECT_EQ(1u, chrome::GetTotalBrowserCount());
  EXPECT_EQ(nullptr, FindAIChatBrowser());
}

IN_PROC_BROWSER_TEST_F(AIChatAgentProfileStartupBrowserTest,
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

IN_PROC_BROWSER_TEST_F(AIChatAgentProfileStartupBrowserTest,
                       ProfileNotReopenedOnStartup) {
  // Verify the AI Chat profile is not opened on startup
  // This tests the override in startup_browser_creator.cc.
  EXPECT_EQ(1u, chrome::GetTotalBrowserCount());
  EXPECT_EQ(nullptr, FindAIChatBrowser());
}

}  // namespace ai_chat
