// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "base/test/scoped_feature_list.h"
#include "base/test/test_future.h"
#include "brave/browser/ai_chat/ai_chat_agent_profile_helper.h"
#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/side_panel/side_panel.h"
#include "chrome/browser/ui/views/side_panel/side_panel_coordinator.h"
#include "chrome/browser/ui/views/side_panel/side_panel_web_ui_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "printing/buildflags/buildflags.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/mojom/window_features/window_features.mojom.h"
#include "ui/views/controls/webview/webview.h"
#include "url/gurl.h"

// Tests sidepanel behavior for AI Chat scenarios
class AIChatGlobalSidePanelBrowserTest
    : public InProcessBrowserTest,
      public testing::WithParamInterface<bool> {
 public:
  AIChatGlobalSidePanelBrowserTest() {
    std::vector<base::test::FeatureRef> enabled_features = {
        ai_chat::features::kAIChatAgentProfile};
    std::vector<base::test::FeatureRef> disabled_features = {};

    bool global_flag_enabled = GetParam();

    if (global_flag_enabled) {
      enabled_features.push_back(
          ai_chat::features::kAIChatGlobalSidePanelEverywhere);
    } else {
      disabled_features.push_back(
          ai_chat::features::kAIChatGlobalSidePanelEverywhere);
    }

    scoped_feature_list_.InitWithFeatures(enabled_features, disabled_features);
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    // Must be opted-in to use AI Chat agent profile
    ai_chat::SetUserOptedIn(browser()->profile()->GetPrefs(), true);
  }

  ~AIChatGlobalSidePanelBrowserTest() override = default;

  bool IsGlobalFlagEnabled() const { return GetParam(); }

 protected:
  void OpenSidePanelAndVerify(Browser* browser) {
    auto* side_panel_coordinator = SidePanelCoordinator::From(browser);
    ASSERT_TRUE(side_panel_coordinator);

    side_panel_coordinator->Show(SidePanelEntry::Id::kChatUI);

    auto* side_panel_web_contents =
        side_panel_coordinator->GetWebContentsForTest(
            SidePanelEntry::Id::kChatUI);
    ASSERT_TRUE(side_panel_web_contents);

    content::WaitForLoadStop(side_panel_web_contents);
  }

  bool IsSidePanelOpen(Browser* browser) {
    auto* side_panel_coordinator = SidePanelCoordinator::From(browser);
    if (!side_panel_coordinator) {
      return false;
    }

    return side_panel_coordinator->IsSidePanelShowing(
               SidePanelEntry::PanelType::kContent) &&
           side_panel_coordinator->GetCurrentEntryId(
               SidePanelEntry::PanelType::kContent) ==
               SidePanelEntry::Id::kChatUI;
  }

  bool IsGlobalSidePanel(Browser* browser) {
    // Test global behavior by checking if sidepanel stays open when switching
    // tabs
    auto* side_panel_coordinator = SidePanelCoordinator::From(browser);
    EXPECT_TRUE(side_panel_coordinator);

    // Ensure we have at least one tab
    EXPECT_GE(browser->tab_strip_model()->count(), 1);

    // Open sidepanel on current tab
    side_panel_coordinator->Show(SidePanelEntry::Id::kChatUI);
    EXPECT_TRUE(IsSidePanelOpen(browser));

    // Create a new tab
    GURL test_url("chrome://version/");
    ui_test_utils::NavigateToURLWithDisposition(
        browser, test_url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
        ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);

    // Wait for tab to be active
    EXPECT_GE(browser->tab_strip_model()->count(), 2);
    EXPECT_EQ(browser->tab_strip_model()->active_index(), 1);

    // Check if sidepanel is still open after tab switch
    // Global sidepanel stays open, per-tab sidepanel closes
    bool stays_open_after_tab_switch = IsSidePanelOpen(browser);

    // Switch back to first tab
    browser->tab_strip_model()->ActivateTabAt(0);
    EXPECT_EQ(browser->tab_strip_model()->active_index(), 0);

    // Clean up - close the extra tab
    browser->tab_strip_model()->CloseWebContentsAt(1, 0);
    EXPECT_EQ(browser->tab_strip_model()->count(), 1);

    return stays_open_after_tab_switch;
  }

  base::test::ScopedFeatureList scoped_feature_list_;
};

// Test sidepanel behavior in regular windows
IN_PROC_BROWSER_TEST_P(AIChatGlobalSidePanelBrowserTest,
                       GlobalSidePanelBehavior) {
  // Sidepanel for regular browser should have global behavior if feature flag
  // is enabled.
  bool expected_global_behavior = IsGlobalFlagEnabled();
  EXPECT_EQ(expected_global_behavior, IsGlobalSidePanel(browser()));

  // Regardless of feature flag, AI Chat agent profile browser should always
  // have global sidepanel behavior.
  base::test::TestFuture<Browser*> ai_chat_browser_future;
  ai_chat::OpenBrowserWindowForAIChatAgentProfileForTesting(
      *browser()->profile(), ai_chat_browser_future.GetCallback());
  Browser* ai_chat_browser = ai_chat_browser_future.Get();
  ASSERT_TRUE(ai_chat_browser);
  ASSERT_TRUE(ai_chat_browser->profile()->IsAIChatAgent());

  // Test that agent profile always uses global behavior regardless of flag
  // state
  EXPECT_TRUE(IsGlobalSidePanel(ai_chat_browser));
}

// Test that AddNewContents (triggered when a link is clicked in the AI Chat
// panel) opens the URL in the current tab when the global side panel is
// enabled, or in a new tab when it is disabled.
IN_PROC_BROWSER_TEST_P(AIChatGlobalSidePanelBrowserTest,
                       AddNewContentsUsesCorrectDisposition) {
  auto* side_panel_coordinator = SidePanelCoordinator::From(browser());
  ASSERT_TRUE(side_panel_coordinator);

  side_panel_coordinator->Show(SidePanelEntry::Id::kChatUI);

  // GetWebContentsForTest calls entry->GetContent() which, after Show() has
  // consumed the content view, invokes the factory again and returns a fresh
  // unattached view. Its WebContents has no widget, so calling AddNewContents
  // on it would crash. Instead, find the WebContents from the view that is
  // actually attached to the browser window.
  auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser());
  ASSERT_TRUE(browser_view);
  auto* side_panel = browser_view->contents_height_side_panel();
  ASSERT_TRUE(side_panel);
  auto* view = side_panel->GetContentParentView()->GetViewByID(
      SidePanelWebUIView::kSidePanelWebViewId);
  ASSERT_TRUE(view);
  auto* side_panel_web_contents =
      static_cast<views::WebView*>(view)->web_contents();
  ASSERT_TRUE(side_panel_web_contents);
  content::WaitForLoadStop(side_panel_web_contents);

  int initial_tab_count = browser()->tab_strip_model()->count();

  blink::mojom::WindowFeatures window_features;
  bool was_blocked = false;
  side_panel_web_contents->GetDelegate()->AddNewContents(
      side_panel_web_contents, nullptr, GURL("chrome://version/"),
      WindowOpenDisposition::NEW_FOREGROUND_TAB, window_features,
      /*user_gesture=*/true, &was_blocked);

  int final_tab_count = browser()->tab_strip_model()->count();

  if (IsGlobalFlagEnabled()) {
    // Global side panel enabled: link navigates the current tab, no new tab.
    EXPECT_EQ(final_tab_count, initial_tab_count);
  } else {
    // Global side panel disabled: link opens in a new foreground tab.
    EXPECT_EQ(final_tab_count, initial_tab_count + 1);
  }
}

INSTANTIATE_TEST_SUITE_P(
    ,
    AIChatGlobalSidePanelBrowserTest,
    testing::Bool(),
    [](const testing::TestParamInfo<
        AIChatGlobalSidePanelBrowserTest::ParamType>& info) {
      return absl::StrFormat("GlobalSidePanelFeature_%s",
                             info.param ? "Enabled" : "NotEnabled");
    });
