// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "base/containers/fixed_flat_set.h"
#include "base/path_service.h"
#include "base/test/bind.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/ai_chat/ai_chat_agent_profile_helper.h"
#include "brave/browser/ai_chat/ai_chat_service_factory.h"
#include "brave/browser/ui/webui/ai_chat/ai_chat_ui.h"
#include "brave/components/ai_chat/core/browser/ai_chat_service.h"
#include "brave/components/ai_chat/core/browser/conversation_handler.h"
#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "brave/components/brave_origin/brave_origin_policy_manager.h"
#include "brave/components/brave_origin/profile_id.h"
#include "brave/components/constants/brave_constants.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profile_test_util.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/browser_window/public/global_browser_collection.h"
#include "chrome/browser/ui/profiles/profile_picker.h"
#include "chrome/browser/ui/profiles/profile_view_utils.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/side_panel/side_panel_coordinator.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/webui_url_constants.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/policy/policy_constants.h"
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
    auto* side_panel_coordinator = SidePanelCoordinator::From(browser);
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
    Browser* browser_found = nullptr;
    GlobalBrowserCollection::GetInstance()->ForEach(
        [&browser_found](BrowserWindowInterface* browser) {
          if (browser->GetProfile()->IsAIChatAgent()) {
            browser_found = browser->GetBrowserForMigrationOnly();
          }
          return browser_found == nullptr;
        });
    return browser_found;
  }

  Profile* GetProfile() { return browser()->profile(); }

  base::test::ScopedFeatureList scoped_feature_list_;
};

// Test that OpenBrowserWindowForAIChatAgentProfile creates a browser window
IN_PROC_BROWSER_TEST_F(AIChatAgentProfileBrowserTest,
                       OpenBrowserWindowForAIChatAgentProfile) {
  // Keep track of initial browser count
  EXPECT_EQ(1u, GlobalBrowserCollection::GetInstance()->GetSize());

  // The source profile is opted in to AI Chat, so the new agent profile should
  // inherit the opt-in.
  SetUserOptedIn(GetProfile()->GetPrefs(), true);

  // Request to open AI Chat Agent Profile browser window should open a new
  // browser window.
  Browser* opened_browser =
      CallOpenBrowserWindowForAiChatAgentProfile(GetProfile());

  // Verify that a new browser window was opened
  EXPECT_EQ(2u, GlobalBrowserCollection::GetInstance()->GetSize());

  // Find the AI Chat browser
  Browser* ai_chat_browser = FindAIChatBrowser();
  ASSERT_TRUE(ai_chat_browser);
  EXPECT_EQ(opened_browser, ai_chat_browser);

  // The agent profile should have inherited the opt-in from the source profile.
  EXPECT_TRUE(HasUserOptedIn(ai_chat_browser->profile()->GetPrefs()));

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
  EXPECT_EQ(u"AI browsing", profile_attributes->GetLocalProfileName());

  // Verify the AI Chat browser has the side panel opened to Chat UI
  VerifyAIChatSidePanelShowing(ai_chat_browser);

  // Verify the new tab page is the AI Chat Agent new tab page
  auto* ntp_rfh = ui_test_utils::NavigateToURL(
      ai_chat_browser, GURL(chrome::kChromeUINewTabURL));
  ASSERT_TRUE(ntp_rfh);
  EXPECT_TRUE(content::EvalJs(ntp_rfh,
                              "!!document.querySelector(`html[data-testid="
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
    EXPECT_TRUE(kContentAgentToolNames.contains(tool->Name()))
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
      EXPECT_FALSE(kContentAgentToolNames.contains(tool->Name()))
          << "Tool " << tool->Name() << " should not be in the regular profile";
    }
  }
}

// Test that opening the AI Chat Agent Profile is allowed even when the source
// profile has not opted in, but in that case the agent profile is not opted in
// either.
IN_PROC_BROWSER_TEST_F(AIChatAgentProfileBrowserTest,
                       OpenBrowserWindowForAIChatAgentProfile_NotOptedIn) {
  EXPECT_EQ(1u, GlobalBrowserCollection::GetInstance()->GetSize());
  ASSERT_FALSE(HasUserOptedIn(GetProfile()->GetPrefs()));

  // Opening the agent profile is allowed even when the source profile has not
  // opted in to AI Chat.
  Browser* opened_browser =
      CallOpenBrowserWindowForAiChatAgentProfile(GetProfile());
  ASSERT_TRUE(opened_browser);
  EXPECT_EQ(2u, GlobalBrowserCollection::GetInstance()->GetSize());

  Browser* ai_chat_browser = FindAIChatBrowser();
  ASSERT_TRUE(ai_chat_browser);
  EXPECT_EQ(opened_browser, ai_chat_browser);
  EXPECT_TRUE(ai_chat_browser->profile()->IsAIChatAgent());

  // Since no other profile has opted in, the agent profile should not be opted
  // in either; the user should opt in via the agent profile's own flow.
  EXPECT_FALSE(HasUserOptedIn(ai_chat_browser->profile()->GetPrefs()));
}

// Test that multiple calls to OpenBrowserWindowForAIChatAgentProfile work
// correctly
IN_PROC_BROWSER_TEST_F(AIChatAgentProfileBrowserTest,
                       OpenBrowserWindowForAIChatAgentProfile_MultipleOpens) {
  SetUserOptedIn(GetProfile()->GetPrefs(), true);

  EXPECT_EQ(1u, GlobalBrowserCollection::GetInstance()->GetSize());

  // First call to open AI Chat profile
  Browser* opened_browser =
      CallOpenBrowserWindowForAiChatAgentProfile(GetProfile());
  ASSERT_TRUE(opened_browser);
  EXPECT_EQ(2u, GlobalBrowserCollection::GetInstance()->GetSize());

  EXPECT_EQ(opened_browser, FindAIChatBrowser());

  // Second call should not open a new one
  Browser* second_opened_browser =
      CallOpenBrowserWindowForAiChatAgentProfile(GetProfile());
  ASSERT_TRUE(second_opened_browser);
  EXPECT_EQ(opened_browser, second_opened_browser);
  EXPECT_EQ(2u, GlobalBrowserCollection::GetInstance()->GetSize());

  VerifyAIChatSidePanelShowing(opened_browser);

  // Close browser
  CloseBrowserSynchronously(opened_browser);
  EXPECT_EQ(1u, GlobalBrowserCollection::GetInstance()->GetSize());

  // Subsequent call to open should open a new browser
  Browser* third_opened_browser =
      CallOpenBrowserWindowForAiChatAgentProfile(GetProfile());
  ASSERT_TRUE(third_opened_browser);
  EXPECT_EQ(2u, GlobalBrowserCollection::GetInstance()->GetSize());
  EXPECT_EQ(third_opened_browser, FindAIChatBrowser());

  VerifyAIChatSidePanelShowing(third_opened_browser);
}

// Regression test for a crash seen on Brave Origin installs. Brave Origin
// manages AI Chat as a profile-scoped policy that defaults to disabled for
// newly-created profiles. When the source profile has AI Chat enabled but the
// freshly-created agent profile inherits the default-disabled policy, the agent
// profile's AIChatService is null, the kChatUI side panel entry is never
// registered, and showing it used to CHECK-crash. The agent profile must
// instead inherit the source profile's AI Chat policy value.
IN_PROC_BROWSER_TEST_F(AIChatAgentProfileBrowserTest,
                       OpenBrowserWindowInheritsAIChatPolicyUnderBraveOrigin) {
  auto* origin_manager = brave_origin::BraveOriginPolicyManager::GetInstance();
  ASSERT_TRUE(origin_manager->IsInitialized());

  // Mark the source profile's AI Chat policy as enabled, like a Brave Origin
  // user who has kept AI Chat on. New profiles still default to disabled.
  origin_manager->SetPolicyValue(
      policy::key::kBraveAIChatEnabled, true,
      brave_origin::GetProfileId(GetProfile()->GetPath()));

  // Re-assert the purchased state inside the wait loop: the source profile's
  // BraveOriginService runs a SKU purchase check at startup that can reset it
  // (there is no real purchase in tests), so keep setting it until the managed
  // AI Chat policy has propagated to the source profile.
  ASSERT_TRUE(base::test::RunUntil([&]() {
    origin_manager->SetPurchased(true);
    return GetProfile()->GetPrefs()->IsManagedPreference(
               prefs::kEnabledByPolicy) &&
           IsAIChatEnabled(GetProfile()->GetPrefs());
  }));

  SetUserOptedIn(GetProfile()->GetPrefs(), true);

  // Before the fix this crashes while showing the agent profile's side panel.
  Browser* agent_browser =
      CallOpenBrowserWindowForAiChatAgentProfile(GetProfile());
  ASSERT_TRUE(agent_browser);
  ASSERT_TRUE(agent_browser->profile()->IsAIChatAgent());

  // The agent profile must have inherited the source profile's enabled AI Chat
  // policy, so the service is available and the side panel can be shown.
  EXPECT_TRUE(IsAIChatEnabled(agent_browser->profile()->GetPrefs()));
  EXPECT_NE(
      AIChatServiceFactory::GetForBrowserContext(agent_browser->profile()),
      nullptr);
  VerifyAIChatSidePanelShowing(agent_browser);
}

// Regression test for existing users: an agent profile created before this fix
// shipped has AI Chat left disabled in its persisted Brave Origin policy.
// Opening such a profile must repair it (re-enable AI Chat) rather than crash.
// This pre-seeds the agent profile's stored policy as disabled to simulate that
// state, since browser tests retain profiles in memory and cannot reload an
// existing profile within a single test.
IN_PROC_BROWSER_TEST_F(AIChatAgentProfileBrowserTest,
                       DisabledAgentProfilePolicyIsRepairedUnderBraveOrigin) {
  auto* origin_manager = brave_origin::BraveOriginPolicyManager::GetInstance();
  ASSERT_TRUE(origin_manager->IsInitialized());

  // The source profile has AI Chat enabled.
  origin_manager->SetPolicyValue(
      policy::key::kBraveAIChatEnabled, true,
      brave_origin::GetProfileId(GetProfile()->GetPath()));

  // Simulate an agent profile created before this fix: its persisted Brave
  // Origin policy has AI Chat explicitly disabled.
  const base::FilePath agent_path =
      base::PathService::CheckedGet(chrome::DIR_USER_DATA)
          .Append(brave::kAIChatAgentProfileDir);
  origin_manager->SetPolicyValue(policy::key::kBraveAIChatEnabled, false,
                                 brave_origin::GetProfileId(agent_path));

  ASSERT_TRUE(base::test::RunUntil([&]() {
    origin_manager->SetPurchased(true);
    return GetProfile()->GetPrefs()->IsManagedPreference(
               prefs::kEnabledByPolicy) &&
           IsAIChatEnabled(GetProfile()->GetPrefs());
  }));

  // Opening the agent profile must repair the disabled state and not crash.
  Browser* agent_browser =
      CallOpenBrowserWindowForAiChatAgentProfile(GetProfile());
  ASSERT_TRUE(agent_browser);
  ASSERT_TRUE(agent_browser->profile()->IsAIChatAgent());
  ASSERT_EQ(agent_browser->profile()->GetPath(), agent_path);
  EXPECT_TRUE(IsAIChatEnabled(agent_browser->profile()->GetPrefs()));
  VerifyAIChatSidePanelShowing(agent_browser);
}

// The agent profile manager observes every profile, so guard against it
// accidentally enabling AI Chat for regular (non-agent) profiles: only the
// dedicated agent profile should be enabled. A regular profile must keep Brave
// Origin's default and never have its AI Chat policy written by the manager.
IN_PROC_BROWSER_TEST_F(AIChatAgentProfileBrowserTest,
                       RegularProfileNotEnabledForAIChatByAgentManager) {
  auto* origin_manager = brave_origin::BraveOriginPolicyManager::GetInstance();
  ASSERT_TRUE(origin_manager->IsInitialized());
  origin_manager->SetPurchased(true);

  // Create a regular, non-agent profile. The agent profile manager's
  // OnProfileAdded() observer fires for it during creation.
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  Profile& regular_profile = profiles::testing::CreateProfileSync(
      profile_manager,
      profile_manager->user_data_dir().AppendASCII("RegularTestProfile"));
  ASSERT_FALSE(regular_profile.IsAIChatAgent());

  // The manager must not have written an AI Chat policy value for the regular
  // profile; it keeps Brave Origin's default (disabled).
  EXPECT_FALSE(origin_manager
                   ->GetPolicyValue(
                       policy::key::kBraveAIChatEnabled,
                       brave_origin::GetProfileId(regular_profile.GetPath()))
                   .value_or(false));
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
        SidePanelCoordinator::From(browser)->GetWebContentsForTest(
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
        SidePanelCoordinator::From(browser)->GetWebContentsForTest(
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
                                "[title=\"Open AI browsing profile window\"]");
  }

  void WaitForAIChatAgentProfileLaunchButton(Browser* browser) {
    WaitForElementInSidePanel(browser,
                              "[title=\"Open AI browsing profile window\"]");
  }
};

IN_PROC_BROWSER_TEST_P(AIChatAgentProfileWebUIContentBrowserTest,
                       AgentProfileElements) {
  bool feature_enabled = GetParam();
  EXPECT_EQ(1u, GlobalBrowserCollection::GetInstance()->GetSize());
  EXPECT_FALSE(GetProfile()->IsAIChatAgent());

  VerifyAIChatSidePanelShowing(browser(), true);
  WaitForAIChatRender(browser());

  if (!feature_enabled) {
    // When the feature is disabled, no buttons are shown
    EXPECT_FALSE(IsAIChatAgentProfileLaunchButtonPresent(browser()));
    EXPECT_FALSE(IsAIChatAgentProfileTooltipPresent(browser()));
    return;
  }

  // The agent profile launch button is shown even when not opted in, so that
  // the user can start the agent profile and opt in there.
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
  EXPECT_EQ(2u, GlobalBrowserCollection::GetInstance()->GetSize());

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
  EXPECT_EQ(1u, GlobalBrowserCollection::GetInstance()->GetSize());
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
  EXPECT_EQ(2u, GlobalBrowserCollection::GetInstance()->GetSize());

  // Simulate the profile picker having been shown without the user
  // unchecking the "Show profile picker on startup" checkbox.
  // This tests where the user previously had multiple profiles but
  // now only has one regular profile and one AI Chat agent profile.
  // Since they would have seen the profile picker before, this pref
  // will be true.
  g_browser_process->local_state()->SetBoolean(
      ::prefs::kBrowserProfilePickerShown, true);

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
  EXPECT_EQ(1u, GlobalBrowserCollection::GetInstance()->GetSize());
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
  EXPECT_EQ(2u, GlobalBrowserCollection::GetInstance()->GetSize());
  EXPECT_EQ(opened_browser, FindAIChatBrowser());
  // Leave the browser windows open
}

IN_PROC_BROWSER_TEST_F(AIChatAgentProfileStartupBrowserTest,
                       ProfileNotReopenedOnStartup) {
  // Verify the AI Chat profile is not opened on startup
  // This tests the override in startup_browser_creator.cc.
  EXPECT_EQ(1u, GlobalBrowserCollection::GetInstance()->GetSize());
  EXPECT_EQ(nullptr, FindAIChatBrowser());
}

}  // namespace ai_chat
