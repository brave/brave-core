// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <tuple>

#include "base/memory/scoped_refptr.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/test_future.h"
#include "base/time/time.h"
#include "brave/browser/ai_chat/ai_chat_agent_profile_helper.h"
#include "brave/browser/ui/side_panel/ai_chat/ai_chat_side_panel_utils.h"
#include "brave/browser/ui/views/side_panel/ai_chat/ai_chat_movable_side_panel_web_view.h"
#include "brave/browser/ui/views/side_panel/ai_chat/ai_chat_side_panel_web_view.h"
#include "brave/browser/ui/webui/ai_chat/ai_chat_ui.h"
#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/animation/browser_animation_controller.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/animations/side_panel_animations.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/layout/browser_view_layout.h"
#include "chrome/browser/ui/views/side_panel/side_panel.h"
#include "chrome/browser/ui/views/side_panel/side_panel_coordinator.h"
#include "chrome/browser/ui/views/side_panel/side_panel_web_ui_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_delegate.h"
#include "content/public/browser/web_ui.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "printing/buildflags/buildflags.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/mojom/window_features/window_features.mojom.h"
#include "ui/gfx/animation/animation_container.h"
#include "ui/gfx/animation/animation_test_api.h"
#include "ui/views/controls/webview/webview.h"
#include "ui/views/view_utils.h"
#include "url/gurl.h"

// Tests sidepanel behavior for AI Chat scenarios
class AIChatGlobalSidePanelBrowserTest
    : public InProcessBrowserTest,
      public testing::WithParamInterface<std::tuple<bool, bool>> {
 public:
  AIChatGlobalSidePanelBrowserTest() {
    std::vector<base::test::FeatureRef> enabled_features = {
        ai_chat::features::kAIChatAgentProfile};
    std::vector<base::test::FeatureRef> disabled_features = {};

    // The global side panel and the move-full-page-to-side-panel features are
    // parameterized independently so the same assertions run against both the
    // wrapper-based side panel view (move feature off) and the movable plain
    // WebView (move feature on).
    (IsGlobalFlagEnabled() ? enabled_features : disabled_features)
        .push_back(ai_chat::features::kAIChatGlobalSidePanelEverywhere);
    (IsMoveToSidePanelEnabled() ? enabled_features : disabled_features)
        .push_back(ai_chat::features::kAIChatMoveFullPageToSidePanel);

    scoped_feature_list_.InitWithFeatures(enabled_features, disabled_features);
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    // Must be opted-in to use AI Chat agent profile
    ai_chat::SetUserOptedIn(browser()->profile()->GetPrefs(), true);
  }

  ~AIChatGlobalSidePanelBrowserTest() override = default;

  bool IsGlobalFlagEnabled() const { return std::get<0>(GetParam()); }
  bool IsMoveToSidePanelEnabled() const { return std::get<1>(GetParam()); }

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

    return side_panel_coordinator->IsSidePanelShowing() &&
           side_panel_coordinator->GetCurrentEntryId() ==
               SidePanelEntry::Id::kChatUI;
  }

  // Returns the WebContents of the side panel view currently attached to the
  // browser window (as opposed to `GetWebContentsForTest`, which re-runs the
  // entry factory and returns a fresh, unattached contents). Null if the side
  // panel is not hosting an AI Chat view.
  content::WebContents* GetAttachedSidePanelWebContents(Browser* browser) {
    auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser);
    if (!browser_view) {
      return nullptr;
    }
    auto* view =
        browser_view->GetViewByID(SidePanelWebUIView::kSidePanelWebViewId);
    return view ? views::AsViewClass<views::WebView>(view)->web_contents()
                : nullptr;
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
  auto* side_panel = browser_view->side_panel();
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

// The kChatUI side panel hosts and loads the AI Chat WebUI regardless of which
// view backs it: the wrapper-based `AIChatSidePanelWebView` when the move
// feature is off, or the movable plain `AIChatMovableSidePanelWebView` when it
// is on. This verifies the conversation UI still opens and is reachable through
// the standard side panel lookups for the movable view.
IN_PROC_BROWSER_TEST_P(AIChatGlobalSidePanelBrowserTest,
                       SidePanelHostsAndLoadsAIChatUI) {
  auto* side_panel_coordinator = SidePanelCoordinator::From(browser());
  ASSERT_TRUE(side_panel_coordinator);

  side_panel_coordinator->Show(SidePanelEntry::Id::kChatUI);
  EXPECT_TRUE(IsSidePanelOpen(browser()));

  auto* side_panel_web_contents = side_panel_coordinator->GetWebContentsForTest(
      SidePanelEntry::Id::kChatUI);
  ASSERT_TRUE(side_panel_web_contents);
  ASSERT_TRUE(content::WaitForLoadStop(side_panel_web_contents));

  auto* web_ui = side_panel_web_contents->GetWebUI();
  ASSERT_TRUE(web_ui);
  EXPECT_TRUE(web_ui->GetController()->GetAs<AIChatUI>());
}

// Moving a full-page AI Chat detaches its live WebContents from the tab strip
// and shows it in the side panel (preserving state). The helper is only
// responsible for the move; opening the clicked link is the caller's separate
// responsibility (AIChatUIPageHandler::OpenURLInNewTab). The move only happens
// with the global (window-scoped) side panel; when the move feature is disabled
// or the side panel is tab-scoped, the helper is a no-op and AI Chat stays a
// full tab.
IN_PROC_BROWSER_TEST_P(AIChatGlobalSidePanelBrowserTest,
                       ForwardMovesFullPageChatToSidePanel) {
  auto* tab_strip = browser()->tab_strip_model();

  // Open the full-page AI Chat conversation in a new tab, keeping the original
  // tab so the tab strip stays valid after AI Chat is detached.
  ASSERT_TRUE(ui_test_utils::NavigateToURLWithDisposition(
      browser(), GURL(kAIChatUIURL), WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP));
  content::WebContents* leo_contents = tab_strip->GetActiveWebContents();
  ASSERT_TRUE(leo_contents);

  const int initial_tab_count = tab_strip->count();

  // The transfer requires both the move feature and the global side panel.
  const bool expect_transfer =
      IsMoveToSidePanelEnabled() && IsGlobalFlagEnabled();

  const bool handled = ai_chat::MaybeMoveFullPageChatToSidePanel(leo_contents);

  if (!expect_transfer) {
    // No transfer: AI Chat stays a full tab.
    EXPECT_FALSE(handled);
    EXPECT_NE(tab_strip->GetIndexOfWebContents(leo_contents),
              TabStripModel::kNoTab);
    EXPECT_EQ(tab_strip->count(), initial_tab_count);
    return;
  }

  EXPECT_TRUE(handled);

  // AI Chat's live contents left the tab strip. The move itself opens no tab
  // for the link, so the tab count drops by one.
  EXPECT_EQ(tab_strip->GetIndexOfWebContents(leo_contents),
            TabStripModel::kNoTab);
  EXPECT_EQ(tab_strip->count(), initial_tab_count - 1);

  // The AI Chat side panel now shows and hosts the *same* live WebContents (no
  // reload / no fresh contents).
  ASSERT_TRUE(
      base::test::RunUntil([&]() { return IsSidePanelOpen(browser()); }));
  EXPECT_EQ(GetAttachedSidePanelWebContents(browser()), leo_contents);
}

// The forward move slides the conversation into the side panel with a content
// transition (flash-free) rather than a plain open. The side panel only holds
// its content as the browser view's "animation content" when it opens via
// `SidePanelUI::ShowFrom` with a non-empty starting rect, so observing that
// content mid-animation verifies the move captured the full-page bounds and
// routed through `ShowFrom` (and did not regress to a plain `Show`).
IN_PROC_BROWSER_TEST_P(AIChatGlobalSidePanelBrowserTest,
                       ForwardMoveAnimatesConversationIntoSidePanel) {
  if (!(IsMoveToSidePanelEnabled() && IsGlobalFlagEnabled())) {
    GTEST_SKIP() << "The transfer only happens with move + global enabled.";
  }

  auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser());
  ASSERT_TRUE(browser_view);
  auto* side_panel = browser_view->side_panel();
  ASSERT_TRUE(side_panel);

  auto* coordinator = SidePanelCoordinator::From(browser());
  ASSERT_TRUE(coordinator);
  // Show the panel content synchronously so the open animation starts as soon
  // as the move shows the panel.
  coordinator->SetNoDelaysForTesting(true);

  // Drive the side panel open animation manually so it can be observed
  // mid-flight. Constructing the test API pauses the container's runner.
  auto* animation_controller = BrowserAnimationController::From(browser());
  auto animation_container = base::MakeRefCounted<gfx::AnimationContainer>();
  animation_controller->SetAnimationContainerForTesting(
      SidePanelAnimations::kSidePanel, animation_container.get());
  gfx::AnimationContainerTestApi animation_test_api(animation_container.get());

  // Open a full-page AI Chat conversation in a new tab, keeping the original
  // tab so the tab strip stays valid after AI Chat is detached.
  auto* tab_strip = browser()->tab_strip_model();
  ASSERT_TRUE(ui_test_utils::NavigateToURLWithDisposition(
      browser(), GURL(kAIChatUIURL), WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP));
  content::WebContents* leo_contents = tab_strip->GetActiveWebContents();
  ASSERT_TRUE(leo_contents);

  ASSERT_TRUE(ai_chat::MaybeMoveFullPageChatToSidePanel(leo_contents));

  // Part-way through the open, the conversation is animating in as the side
  // panel's content and has not yet been reparented into the panel's content
  // area. A plain (non-`ShowFrom`) open would never set this.
  ASSERT_TRUE(base::test::RunUntil([&]() {
    return browser_view->GetBrowserViewLayoutForTesting()
               ->side_panel_animation_content() != nullptr;
  }));

  // When the animation completes the conversation is reparented into the side
  // panel, which hosts the same live WebContents (no reload / fresh contents).
  animation_test_api.IncrementTime(base::Milliseconds(500));
  EXPECT_EQ(browser_view->GetBrowserViewLayoutForTesting()
                ->side_panel_animation_content(),
            nullptr);
  ASSERT_EQ(side_panel->GetContentParentView()->children().size(), 1u);
  EXPECT_EQ(GetAttachedSidePanelWebContents(browser()), leo_contents);
}

// The forward move only applies to a full-page AI Chat. When AI Chat is already
// hosted in the side panel, its contents is not a tab, so the helper is a
// no-op.
IN_PROC_BROWSER_TEST_P(AIChatGlobalSidePanelBrowserTest,
                       ForwardMoveIsNoOpWhenChatAlreadyInSidePanel) {
  OpenSidePanelAndVerify(browser());
  ASSERT_TRUE(IsSidePanelOpen(browser()));

  content::WebContents* side_panel_contents =
      GetAttachedSidePanelWebContents(browser());
  ASSERT_TRUE(side_panel_contents);

  const int initial_tab_count = browser()->tab_strip_model()->count();

  EXPECT_FALSE(ai_chat::MaybeMoveFullPageChatToSidePanel(side_panel_contents));
  EXPECT_EQ(browser()->tab_strip_model()->count(), initial_tab_count);
  EXPECT_TRUE(IsSidePanelOpen(browser()));
}

// Transferring a full-page AI Chat while the side panel already shows an AI
// Chat conversation replaces the panel's contents with the transferred one.
// This exercises the transfer bridge's close-and-reshow path, which rebuilds
// the side panel view so it adopts the moved-in WebContents.
IN_PROC_BROWSER_TEST_P(AIChatGlobalSidePanelBrowserTest,
                       ForwardMoveReplacesExistingSidePanelChat) {
  auto* tab_strip = browser()->tab_strip_model();

  // Show an AI Chat conversation in the side panel first.
  OpenSidePanelAndVerify(browser());
  ASSERT_TRUE(IsSidePanelOpen(browser()));
  content::WebContents* original_panel_contents =
      GetAttachedSidePanelWebContents(browser());
  ASSERT_TRUE(original_panel_contents);

  // Open a separate full-page AI Chat conversation in a new tab.
  ASSERT_TRUE(ui_test_utils::NavigateToURLWithDisposition(
      browser(), GURL(kAIChatUIURL), WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP));
  content::WebContents* full_page_contents = tab_strip->GetActiveWebContents();
  ASSERT_TRUE(full_page_contents);
  ASSERT_NE(full_page_contents, original_panel_contents);

  const bool handled =
      ai_chat::MaybeMoveFullPageChatToSidePanel(full_page_contents);

  // The transfer requires both the move feature and the global side panel.
  if (!(IsMoveToSidePanelEnabled() && IsGlobalFlagEnabled())) {
    EXPECT_FALSE(handled);
    return;
  }

  EXPECT_TRUE(handled);
  EXPECT_EQ(tab_strip->GetIndexOfWebContents(full_page_contents),
            TabStripModel::kNoTab);

  // The side panel now hosts the transferred full-page conversation, replacing
  // the conversation it showed before.
  ASSERT_TRUE(base::test::RunUntil([&]() {
    return IsSidePanelOpen(browser()) &&
           GetAttachedSidePanelWebContents(browser()) == full_page_contents;
  }));
}

// Regression test for the tab-scoped side panel. With the global side panel
// disabled the panel is tied to a tab, so moving a conversation into it and
// then opening the clicked link (which activates a new tab) would close the
// panel and destroy the conversation. The move is therefore skipped in this
// mode: the conversation stays a full-page tab and nothing is lost, even after
// another tab is activated (as opening the link would do).
IN_PROC_BROWSER_TEST_P(AIChatGlobalSidePanelBrowserTest,
                       ForwardMoveKeepsFullPageWhenSidePanelIsContextual) {
  if (IsGlobalFlagEnabled() || !IsMoveToSidePanelEnabled()) {
    GTEST_SKIP()
        << "Only applies to a contextual side panel with move enabled.";
  }

  auto* tab_strip = browser()->tab_strip_model();
  ASSERT_TRUE(ui_test_utils::NavigateToURLWithDisposition(
      browser(), GURL(kAIChatUIURL), WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP));
  content::WebContents* leo_contents = tab_strip->GetActiveWebContents();
  ASSERT_TRUE(leo_contents);
  const int leo_index = tab_strip->GetIndexOfWebContents(leo_contents);

  // The move is skipped for a contextual side panel.
  EXPECT_FALSE(ai_chat::MaybeMoveFullPageChatToSidePanel(leo_contents));

  // AI Chat is untouched: still a full-page tab, and not shown in the panel.
  EXPECT_EQ(tab_strip->GetIndexOfWebContents(leo_contents), leo_index);
  EXPECT_FALSE(IsSidePanelOpen(browser()));

  // Activating another tab (as opening the clicked link would) does not lose
  // the conversation: it survives as a full-page tab.
  ASSERT_TRUE(ui_test_utils::NavigateToURLWithDisposition(
      browser(), GURL("chrome://version/"),
      WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP));
  EXPECT_NE(tab_strip->GetIndexOfWebContents(leo_contents),
            TabStripModel::kNoTab);
}

// Hovering a link in the AI Chat panel must disclose its destination, just like
// a normal tab. The side panel's WebContents delegate does not drive the
// browser status bubble, so each view backing (the wrapper-based
// `AIChatSidePanelWebView` when the move feature is off, and the movable
// `AIChatMovableSidePanelWebView` when it is on) forwards `UpdateTargetURL`
// into its own status bubble. This verifies that path reaches the status bubble
// for whichever view backs the panel, and that leaving a link clears it.
IN_PROC_BROWSER_TEST_P(AIChatGlobalSidePanelBrowserTest,
                       HoveringLinkForwardsTargetURLToStatusBubble) {
  auto* side_panel_coordinator = SidePanelCoordinator::From(browser());
  ASSERT_TRUE(side_panel_coordinator);
  side_panel_coordinator->Show(SidePanelEntry::Id::kChatUI);

  // Find the view actually attached to the browser window (see
  // AddNewContentsUsesCorrectDisposition for why GetWebContentsForTest is not
  // used here).
  auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser());
  ASSERT_TRUE(browser_view);
  auto* side_panel = browser_view->side_panel();
  ASSERT_TRUE(side_panel);
  auto* view = side_panel->GetContentParentView()->GetViewByID(
      SidePanelWebUIView::kSidePanelWebViewId);
  ASSERT_TRUE(view);
  auto* side_panel_web_contents =
      static_cast<views::WebView*>(view)->web_contents();
  ASSERT_TRUE(side_panel_web_contents);
  content::WaitForLoadStop(side_panel_web_contents);

  auto* delegate = side_panel_web_contents->GetDelegate();
  ASSERT_TRUE(delegate);

  // The view by this ID is exactly our concrete side panel view, so the
  // downcast is safe. Which concrete type depends on the move feature.
  auto status_bubble_url = [&]() -> const GURL& {
    return IsMoveToSidePanelEnabled()
               ? static_cast<AIChatMovableSidePanelWebView*>(view)
                     ->status_bubble_url_for_testing()
               : static_cast<AIChatSidePanelWebView*>(view)
                     ->status_bubble_url_for_testing();
  };

  // Hovering a link forwards its destination to the status bubble.
  const GURL hovered("https://example.com/hovered");
  delegate->UpdateTargetURL(side_panel_web_contents, hovered);
  EXPECT_EQ(status_bubble_url(), hovered);

  // Leaving the link (empty URL) clears the status bubble.
  delegate->UpdateTargetURL(side_panel_web_contents, GURL());
  EXPECT_TRUE(status_bubble_url().is_empty());
}

INSTANTIATE_TEST_SUITE_P(
    ,
    AIChatGlobalSidePanelBrowserTest,
    testing::Combine(testing::Bool(), testing::Bool()),
    [](const testing::TestParamInfo<
        AIChatGlobalSidePanelBrowserTest::ParamType>& info) {
      return absl::StrFormat(
          "GlobalSidePanel_%s_MoveToSidePanel_%s",
          std::get<0>(info.param) ? "Enabled" : "NotEnabled",
          std::get<1>(info.param) ? "Enabled" : "NotEnabled");
    });
