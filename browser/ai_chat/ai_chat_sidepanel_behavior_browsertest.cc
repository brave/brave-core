// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include "base/containers/span.h"
#include "base/location.h"
#include "base/memory/scoped_refptr.h"
#include "base/strings/strcat.h"
#include "base/strings/string_split.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/test_future.h"
#include "base/time/time.h"
#include "base/types/expected.h"
#include "brave/browser/ai_chat/ai_chat_agent_profile_helper.h"
#include "brave/browser/ai_chat/ai_chat_conversation_ui_browsertest_base.h"
#include "brave/browser/ai_chat/ai_chat_service_factory.h"
#include "brave/browser/ui/side_panel/ai_chat/ai_chat_side_panel_utils.h"
#include "brave/browser/ui/views/side_panel/ai_chat/ai_chat_movable_side_panel_web_view.h"
#include "brave/browser/ui/views/side_panel/ai_chat/ai_chat_side_panel_web_view.h"
#include "brave/browser/ui/webui/ai_chat/ai_chat_ui.h"
#include "brave/browser/ui/webui/ai_chat/ai_chat_untrusted_conversation_ui.h"
#include "brave/components/ai_chat/core/browser/ai_chat_service.h"
#include "brave/components/ai_chat/core/browser/conversation_handler.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"
#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/ai_chat/core/common/ai_chat_urls.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/animation/browser_animation_controller.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/animations/side_panel_animations.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/layout/browser_view_layout.h"
#include "chrome/browser/ui/views/side_panel/side_panel.h"
#include "chrome/browser/ui/views/side_panel/side_panel_coordinator.h"
#include "chrome/browser/ui/views/side_panel/side_panel_web_ui_view.h"
#include "chrome/common/chrome_isolated_world_ids.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_delegate.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_controller.h"
#include "content/public/common/url_constants.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_navigation_observer.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "printing/buildflags/buildflags.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/animation/animation_container.h"
#include "ui/gfx/animation/animation_test_api.h"
#include "ui/views/controls/webview/webview.h"
#include "url/gurl.h"

// Tests sidepanel behavior for AI Chat scenarios
class AIChatGlobalSidePanelBrowserTest
    : public ai_chat::AIChatConversationUIBrowserTestBase,
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
    ai_chat::SetUserOptedIn(browser()->GetProfile()->GetPrefs(), true);
  }

  ~AIChatGlobalSidePanelBrowserTest() override = default;

  bool IsGlobalFlagEnabled() const { return std::get<0>(GetParam()); }
  bool IsMoveToSidePanelEnabled() const { return std::get<1>(GetParam()); }

 protected:
  void OpenSidePanelAndVerify(BrowserWindowInterface* browser) {
    auto* side_panel_coordinator = SidePanelCoordinator::From(browser);
    ASSERT_TRUE(side_panel_coordinator);

    side_panel_coordinator->Show(SidePanelEntry::Id::kChatUI);

    auto* side_panel_web_contents =
        side_panel_coordinator->GetWebContentsForTest(
            SidePanelEntry::Id::kChatUI);
    ASSERT_TRUE(side_panel_web_contents);

    content::WaitForLoadStop(side_panel_web_contents);
  }

  bool IsSidePanelOpen(BrowserWindowInterface* browser) {
    auto* side_panel_coordinator = SidePanelCoordinator::From(browser);
    if (!side_panel_coordinator) {
      return false;
    }

    return side_panel_coordinator->IsSidePanelShowing() &&
           side_panel_coordinator->GetCurrentEntryId() ==
               SidePanelEntry::Id::kChatUI;
  }

  bool IsGlobalSidePanel(BrowserWindowInterface* browser) {
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
        browser, test_url, WindowOpenDisposition::kNewForegroundTab,
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

  // Creates a fresh, empty conversation via the AIChatService for `browser`'s
  // profile. Returns the conversation uuid (empty only if a precondition EXPECT
  // failed, in which case the test is already failing).
  std::string CreateConversation(BrowserWindowInterface* browser) {
    ai_chat::AIChatService* service =
        ai_chat::AIChatServiceFactory::GetForBrowserContext(
            browser->GetProfile());
    EXPECT_TRUE(service);
    if (!service) {
      return std::string();
    }
    ai_chat::ConversationHandler* conversation = service->CreateConversation();
    EXPECT_TRUE(conversation);
    if (!conversation) {
      return std::string();
    }
    return conversation->get_conversation_uuid();
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
  base::test::TestFuture<BrowserWindowInterface*> ai_chat_browser_future;
  ai_chat::OpenBrowserWindowForAIChatAgentProfileForTesting(
      *browser()->GetProfile(), ai_chat_browser_future.GetCallback());
  BrowserWindowInterface* ai_chat_browser = ai_chat_browser_future.Get();
  ASSERT_TRUE(ai_chat_browser);
  ASSERT_TRUE(ai_chat_browser->GetProfile()->IsAIChatAgent());

  // Test that agent profile always uses global behavior regardless of flag
  // state
  EXPECT_TRUE(IsGlobalSidePanel(ai_chat_browser));
}

// Test that AddNewContents (triggered when a link is clicked in the AI Chat
// panel) opens the URL in the current tab when the global side panel is
// enabled, or in a new tab when it is disabled.
IN_PROC_BROWSER_TEST_P(AIChatGlobalSidePanelBrowserTest,
                       AddNewContentsUsesCorrectDisposition) {
  ASSERT_TRUE(embedded_test_server()->Start());

  auto* side_panel_coordinator = SidePanelCoordinator::From(browser());
  ASSERT_TRUE(side_panel_coordinator);

  side_panel_coordinator->Show(SidePanelEntry::Id::kChatUI);

  // GetWebContentsForTest returns a fresh unattached view, so use the view
  // that is actually attached to the browser window.
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

  auto* tab_strip = browser()->tab_strip_model();
  const int initial_tab_count = tab_strip->count();
  content::WebContents* initial_tab = tab_strip->GetActiveWebContents();

  // Follow a link the way the conversation renders it.
  const GURL link_url = embedded_test_server()->GetURL("/title1.html");
  content::TestNavigationObserver observer(link_url);
  observer.StartWatchingNewWebContents();
  observer.WatchExistingWebContents();
  ASSERT_TRUE(content::ExecJs(
      side_panel_web_contents,
      content::JsReplace("const link = document.createElement('a');"
                         "link.href = $1;"
                         "link.target = '_blank';"
                         "link.rel = 'noopener noreferrer';"
                         "document.body.appendChild(link);"
                         "link.click();",
                         link_url)));
  observer.Wait();

  EXPECT_EQ(tab_strip->GetActiveWebContents()->GetLastCommittedURL(), link_url);
  if (IsGlobalFlagEnabled()) {
    // Global side panel enabled: link navigates the current tab, no new tab.
    EXPECT_EQ(tab_strip->count(), initial_tab_count);
    EXPECT_EQ(tab_strip->GetActiveWebContents(), initial_tab);
  } else {
    // Global side panel disabled: link opens in a new foreground tab.
    EXPECT_EQ(tab_strip->count(), initial_tab_count + 1);
    EXPECT_NE(tab_strip->GetActiveWebContents(), initial_tab);
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
      browser(), GURL(kAIChatUIURL), WindowOpenDisposition::kNewForegroundTab,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP));
  content::WebContents* leo_contents = tab_strip->GetActiveWebContents();
  ASSERT_TRUE(leo_contents);

  ASSERT_TRUE(base::test::RunUntil([&]() {
    return VerifyElementState("standalone-main", true,
                              leo_contents->GetPrimaryMainFrame(), FROM_HERE);
  }));

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
  content::WebContents* side_panel_contents =
      ai_chat::GetSidePanelWebContents(browser());
  ASSERT_TRUE(base::test::RunUntil([&]() {
    return IsSidePanelOpen(browser()) &&
           VerifyElementState("sidepanel-main", true,
                              side_panel_contents->GetPrimaryMainFrame(),
                              FROM_HERE);
  }));
  EXPECT_EQ(side_panel_contents, leo_contents);
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
      browser(), GURL(kAIChatUIURL), WindowOpenDisposition::kNewForegroundTab,
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
  EXPECT_EQ(ai_chat::GetSidePanelWebContents(browser()), leo_contents);
}

// End-to-end forward move: conversation links are plain `<a target="_blank">`
// anchors, so following one opens a new foreground tab through the browser's
// normal new-window path with no AI Chat handler involved.
// `AIChatFullPageLinkObserver` watches for that and moves the full-page
// conversation into the side panel, so the linked page takes its place instead
// of covering it. Where the transfer doesn't apply the link still opens and AI
// Chat stays a full tab. The click is made in the WebUI's main frame: the
// conversation renders its links in an iframe of the same `WebContents`, and
// the browser-side signal the observer watches is identical either way.
IN_PROC_BROWSER_TEST_P(AIChatGlobalSidePanelBrowserTest,
                       LinkOpeningNewTabMovesFullPageChatToSidePanel) {
  ASSERT_TRUE(embedded_test_server()->Start());

  auto* tab_strip = browser()->tab_strip_model();

  // Open the full-page AI Chat conversation in a new tab, keeping the original
  // tab so the tab strip stays valid after AI Chat is detached.
  ASSERT_TRUE(ui_test_utils::NavigateToURLWithDisposition(
      browser(), GURL(kAIChatUIURL), WindowOpenDisposition::kNewForegroundTab,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP));
  content::WebContents* ai_chat_contents = tab_strip->GetActiveWebContents();
  ASSERT_TRUE(ai_chat_contents);

  const int initial_tab_count = tab_strip->count();

  // Follow a link the way the conversation renders it.
  const GURL link_url = embedded_test_server()->GetURL("/title1.html");
  ASSERT_TRUE(content::ExecJs(
      ai_chat_contents,
      content::JsReplace("const link = document.createElement('a');"
                         "link.href = $1;"
                         "link.target = '_blank';"
                         "link.rel = 'noopener noreferrer';"
                         "document.body.appendChild(link);"
                         "link.click();",
                         link_url)));

  // The transfer requires both the move feature and the global side panel.
  if (!(IsMoveToSidePanelEnabled() && IsGlobalFlagEnabled())) {
    // The link opens in a tab of its own and AI Chat stays a full tab.
    ASSERT_TRUE(base::test::RunUntil(
        [&]() { return tab_strip->count() == initial_tab_count + 1; }));
    EXPECT_NE(tab_strip->GetIndexOfWebContents(ai_chat_contents),
              TabStripModel::kNoTab);
    EXPECT_FALSE(IsSidePanelOpen(browser()));
    return;
  }

  // The side panel now shows and hosts the *same* live WebContents (no reload /
  // no fresh contents). Only this end state can be waited on: the link's tab is
  // inserted and AI Chat is detached back-to-back, so the tab strip is never
  // observed holding both.
  ASSERT_TRUE(base::test::RunUntil([&]() {
    return IsSidePanelOpen(browser()) &&
           ai_chat::GetSidePanelWebContents(browser()) == ai_chat_contents;
  }));

  // The link's tab took AI Chat's place: it is the active tab, and the tab
  // count is unchanged because AI Chat left the strip as the link's tab joined.
  EXPECT_EQ(tab_strip->GetIndexOfWebContents(ai_chat_contents),
            TabStripModel::kNoTab);
  EXPECT_EQ(tab_strip->count(), initial_tab_count);
  EXPECT_EQ(tab_strip->GetActiveWebContents()->GetVisibleURL(), link_url);
}

// The forward move only applies to a full-page AI Chat. When AI Chat is already
// hosted in the side panel, its contents is not a tab, so the helper is a
// no-op.
IN_PROC_BROWSER_TEST_P(AIChatGlobalSidePanelBrowserTest,
                       ForwardMoveIsNoOpWhenChatAlreadyInSidePanel) {
  OpenSidePanelAndVerify(browser());
  ASSERT_TRUE(IsSidePanelOpen(browser()));

  content::WebContents* side_panel_contents =
      ai_chat::GetSidePanelWebContents(browser());
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
      ai_chat::GetSidePanelWebContents(browser());
  ASSERT_TRUE(original_panel_contents);

  // Open a separate full-page AI Chat conversation in a new tab.
  ASSERT_TRUE(ui_test_utils::NavigateToURLWithDisposition(
      browser(), GURL(kAIChatUIURL), WindowOpenDisposition::kNewForegroundTab,
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
           ai_chat::GetSidePanelWebContents(browser()) == full_page_contents;
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
      browser(), GURL(kAIChatUIURL), WindowOpenDisposition::kNewForegroundTab,
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
      WindowOpenDisposition::kNewForegroundTab,
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

// The reverse move takes the live AI Chat conversation hosted in the (global)
// side panel and moves it into a full-page tab (preserving state), then closes
// the panel. It only engages with the move feature and the global side panel
// both enabled; otherwise the caller falls back to opening a fresh full-page
// tab and the panel is left untouched.
IN_PROC_BROWSER_TEST_P(AIChatGlobalSidePanelBrowserTest,
                       ReverseMovesSidePanelChatToFullPageTab) {
  // Show the AI Chat side panel and read the *attached* contents (the live
  // panel conversation the reverse move operates on). Avoid
  // `GetWebContentsForTest`, which re-runs the entry factory and returns a
  // fresh, unattached contents rather than the one hosted in the panel.
  auto* coordinator = SidePanelCoordinator::From(browser());
  ASSERT_TRUE(coordinator);
  coordinator->Show(SidePanelEntry::Id::kChatUI);

  // `Show` creates the panel's WebContents and navigates it to the AI Chat
  // WebUI asynchronously, so wait for that navigation to commit and finish
  // before running any script in the panel. Script evaluated in the initial
  // empty document is lost when the WebUI navigation replaces it: the pending
  // promise `VerifyElementState` waits on is destroyed with the document, so
  // `EvalJs` returns an error instead of a value. `IsLoading` is what makes
  // this a post-commit wait (it stays true from navigation start until after
  // commit); `GetWebUI` alone can already be set while the navigation is only
  // starting.
  content::WebContents* panel_contents = nullptr;
  ASSERT_TRUE(base::test::RunUntil([&]() {
    panel_contents = ai_chat::GetSidePanelWebContents(browser());
    return IsSidePanelOpen(browser()) && panel_contents != nullptr &&
           panel_contents->GetWebUI() != nullptr &&
           !panel_contents->IsLoading();
  }));
  ASSERT_TRUE(VerifyElementState("sidepanel-main", /*expect_exist=*/true,
                                 panel_contents->GetPrimaryMainFrame()));

  auto* tab_strip = browser()->tab_strip_model();
  const int initial_tab_count = tab_strip->count();

  ASSERT_TRUE(VerifyElementState("open-full-page-button", /*expect_exist=*/true,
                                 panel_contents->GetPrimaryMainFrame()));
  ASSERT_TRUE(ClickElement("open-full-page-button",
                           panel_contents->GetPrimaryMainFrame()));

  // A new foreground tab should open at the conversation's full-page URL.
  ASSERT_TRUE(base::test::RunUntil([&]() {
    return (browser()->tab_strip_model()->count() == initial_tab_count + 1) &&
           !IsSidePanelOpen(browser());
  }));

  // The transfer requires both the move feature and the global side panel.
  const bool expect_transfer =
      IsMoveToSidePanelEnabled() && IsGlobalFlagEnabled();

  if (!expect_transfer) {
    // No transfer: the conversation is a new web contents.
    EXPECT_EQ(tab_strip->GetIndexOfWebContents(panel_contents),
              TabStripModel::kNoTab);
    content::WebContents* new_tab =
        browser()->tab_strip_model()->GetActiveWebContents();
    ASSERT_TRUE(content::WaitForLoadStop(new_tab));
    const GURL& full_page_url = new_tab->GetLastCommittedURL();
    EXPECT_TRUE(full_page_url.SchemeIs(content::kChromeUIScheme));
    EXPECT_EQ(full_page_url.host(), "leo-ai");
    // The path carries the conversation uuid, e.g. chrome://leo-ai/<uuid>.
    EXPECT_GT(full_page_url.path().length(), 1u);
    return;
  }

  // The same live conversation contents is now a full-page foreground tab (no
  // reload / no fresh contents).
  EXPECT_EQ(tab_strip->GetActiveWebContents(), panel_contents);
  EXPECT_NE(tab_strip->GetIndexOfWebContents(panel_contents),
            TabStripModel::kNoTab);

  // The side panel has closed to avoid showing the same conversation twice.
  // Unlike the wait above, the element check belongs inside the poll here: the
  // conversation's contents is not renavigated by the move, it only swaps the
  // `data-testid` on its existing root element, and `VerifyElementState`
  // observes childList mutations rather than attribute changes. Re-running it
  // is what re-queries for the new id.
  ASSERT_TRUE(base::test::RunUntil([&]() {
    return !IsSidePanelOpen(browser()) &&
           VerifyElementState("standalone-main", true,
                              panel_contents->GetPrimaryMainFrame(), FROM_HERE);
  }));
}

// The reverse move only applies to a side-panel-hosted conversation. A
// full-page AI Chat tab has no side-panel chat view to move, so the helper is a
// no-op regardless of the feature flags and the caller opens a fresh full-page
// tab as before.
IN_PROC_BROWSER_TEST_P(AIChatGlobalSidePanelBrowserTest,
                       ReverseMoveIsNoOpForFullPageChat) {
  auto* tab_strip = browser()->tab_strip_model();
  ASSERT_TRUE(ui_test_utils::NavigateToURLWithDisposition(
      browser(), GURL(kAIChatUIURL), WindowOpenDisposition::kNewForegroundTab,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP));
  content::WebContents* leo_contents = tab_strip->GetActiveWebContents();
  ASSERT_TRUE(leo_contents);
  const int leo_index = tab_strip->GetIndexOfWebContents(leo_contents);

  EXPECT_FALSE(ai_chat::MaybeMoveSidePanelChatToTab(leo_contents));

  // AI Chat is untouched: still a full-page tab at the same position.
  EXPECT_EQ(tab_strip->GetIndexOfWebContents(leo_contents), leo_index);
}

// Round trip: a full-page conversation moved into the side panel (forward) can
// be moved back out to a full-page tab (reverse) as the SAME live WebContents.
// This verifies the reverse works on a moved-in conversation (whose tab
// associations were re-established when the panel adopted it) and that neither
// direction reloads the contents.
IN_PROC_BROWSER_TEST_P(AIChatGlobalSidePanelBrowserTest,
                       ForwardThenReverseRoundTripsSameContents) {
  if (!(IsMoveToSidePanelEnabled() && IsGlobalFlagEnabled())) {
    GTEST_SKIP() << "The transfer only happens with move + global enabled.";
  }

  auto* tab_strip = browser()->tab_strip_model();
  ASSERT_TRUE(ui_test_utils::NavigateToURLWithDisposition(
      browser(), GURL(kAIChatUIURL), WindowOpenDisposition::kNewForegroundTab,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP));
  content::WebContents* leo_contents = tab_strip->GetActiveWebContents();
  ASSERT_TRUE(leo_contents);

  // Forward: full page -> side panel.
  ASSERT_TRUE(ai_chat::MaybeMoveFullPageChatToSidePanel(leo_contents));
  ASSERT_TRUE(base::test::RunUntil([&]() {
    return IsSidePanelOpen(browser()) &&
           ai_chat::GetSidePanelWebContents(browser()) == leo_contents;
  }));
  EXPECT_EQ(tab_strip->GetIndexOfWebContents(leo_contents),
            TabStripModel::kNoTab);

  // Reverse: side panel -> full page, as the same live contents.
  ASSERT_TRUE(ai_chat::MaybeMoveSidePanelChatToTab(leo_contents));
  EXPECT_EQ(tab_strip->GetActiveWebContents(), leo_contents);
  EXPECT_NE(tab_strip->GetIndexOfWebContents(leo_contents),
            TabStripModel::kNoTab);
  ASSERT_TRUE(
      base::test::RunUntil([&]() { return !IsSidePanelOpen(browser()); }));
}

// `OpenConversationInSidePanel(uuid)` opens the (global) AI Chat side panel on
// a specific conversation. When the panel is closed it shows the `kChatUI`
// entry - which creates the WebContents - and navigates that to the
// conversation URL.
IN_PROC_BROWSER_TEST_P(AIChatGlobalSidePanelBrowserTest,
                       OpenSidePanelOpensConversationWhenClosed) {
  if (!IsGlobalFlagEnabled()) {
    GTEST_SKIP()
        << "OpenConversationInSidePanel() only supports the global side panel.";
  }

  auto* coordinator = SidePanelCoordinator::From(browser());
  ASSERT_TRUE(coordinator);
  coordinator->SetNoDelaysForTesting(true);
  ASSERT_FALSE(IsSidePanelOpen(browser()));

  const std::string uuid = CreateConversation(browser());
  ASSERT_FALSE(uuid.empty());
  const GURL conversation_url = ai_chat::ConversationUrl(uuid);

  content::TestNavigationObserver observer(conversation_url);
  observer.StartWatchingNewWebContents();
  ai_chat::OpenConversationInSidePanel(browser()->GetProfile(), uuid);
  observer.Wait();
  EXPECT_TRUE(observer.last_navigation_succeeded());

  EXPECT_TRUE(IsSidePanelOpen(browser()));
  content::WebContents* panel_contents =
      ai_chat::GetSidePanelWebContents(browser());
  ASSERT_TRUE(panel_contents);
  EXPECT_EQ(panel_contents->GetLastCommittedURL(), conversation_url);
}

// When the panel already hosts AI Chat, `OpenConversationInSidePanel()` updates
// the shown conversation in place: it reuses the same live WebContents (no
// fresh contents / no entry rebuild) and navigates it to the requested
// conversation.
IN_PROC_BROWSER_TEST_P(AIChatGlobalSidePanelBrowserTest,
                       OpenSidePanelUpdatesConversationWhenAlreadyOpen) {
  if (!IsGlobalFlagEnabled()) {
    GTEST_SKIP()
        << "OpenConversationInSidePanel() only supports the global side panel.";
  }

  auto* coordinator = SidePanelCoordinator::From(browser());
  ASSERT_TRUE(coordinator);
  coordinator->SetNoDelaysForTesting(true);

  // Open AI Chat first and read the live attached contents (avoid
  // `GetWebContentsForTest`, which re-runs the entry factory and returns a
  // fresh, unattached contents rather than the hosted one).
  coordinator->Show(SidePanelEntry::Id::kChatUI);
  content::WebContents* panel_contents = nullptr;
  ASSERT_TRUE(base::test::RunUntil([&]() {
    panel_contents = ai_chat::GetSidePanelWebContents(browser());
    return IsSidePanelOpen(browser()) && panel_contents != nullptr;
  }));
  ASSERT_TRUE(content::WaitForLoadStop(panel_contents));

  const std::string uuid = CreateConversation(browser());
  ASSERT_FALSE(uuid.empty());
  const GURL conversation_url = ai_chat::ConversationUrl(uuid);

  content::TestNavigationObserver observer(conversation_url);
  observer.WatchExistingWebContents();
  ai_chat::OpenConversationInSidePanel(browser()->GetProfile(), uuid);
  observer.Wait();
  EXPECT_TRUE(observer.last_navigation_succeeded());

  // Same live WebContents, navigated in place to the conversation.
  EXPECT_EQ(ai_chat::GetSidePanelWebContents(browser()), panel_contents);
  EXPECT_EQ(panel_contents->GetLastCommittedURL(), conversation_url);
  EXPECT_TRUE(IsSidePanelOpen(browser()));
}

IN_PROC_BROWSER_TEST_P(AIChatGlobalSidePanelBrowserTest,
                       OpenSidePanelReopensPanelWhileClosing) {
  if (!IsGlobalFlagEnabled()) {
    GTEST_SKIP()
        << "OpenConversationInSidePanel() only supports the global side panel.";
  }

  auto* coordinator = SidePanelCoordinator::From(browser());
  ASSERT_TRUE(coordinator);
  SidePanel* side_panel =
      BrowserView::GetBrowserViewForBrowser(browser())->side_panel();
  ASSERT_TRUE(side_panel);

  const std::string uuid = CreateConversation(browser());
  ASSERT_FALSE(uuid.empty());
  const GURL conversation_url = ai_chat::ConversationUrl(uuid);

  content::TestNavigationObserver observer(conversation_url);
  observer.StartWatchingNewWebContents();
  ai_chat::OpenConversationInSidePanel(browser()->GetProfile(), uuid);
  observer.Wait();
  ASSERT_TRUE(base::test::RunUntil([&]() { return side_panel->GetVisible(); }));

  // Start closing the panel, leaving it in the closing state: the close is
  // animated, so it only completes after the loop is pumped - which this test
  // deliberately does not do before re-opening.
  coordinator->Close();
  ASSERT_TRUE(side_panel->IsClosing());
  ASSERT_TRUE(IsSidePanelOpen(browser()))
      << "the current entry should still be stale-set while closing";

  // Re-opening cancels the close instead of being suppressed by that stale
  // state, and the panel settles open on the same conversation.
  ai_chat::OpenConversationInSidePanel(browser()->GetProfile(), uuid);
  EXPECT_FALSE(side_panel->IsClosing());
  ASSERT_TRUE(base::test::RunUntil([&]() { return side_panel->GetVisible(); }));
  EXPECT_TRUE(IsSidePanelOpen(browser()));
  content::WebContents* panel_contents =
      ai_chat::GetSidePanelWebContents(browser());
  ASSERT_TRUE(panel_contents);
  EXPECT_EQ(panel_contents->GetLastCommittedURL(), conversation_url);
}

// `OpenConversationInSidePanel()` only supports the global side panel. With the
// global side panel disabled (a per-tab / contextual panel) it is a no-op: the
// panel is not opened and nothing is navigated.
IN_PROC_BROWSER_TEST_P(AIChatGlobalSidePanelBrowserTest,
                       OpenSidePanelIsNoOpWhenSidePanelNotGlobal) {
  if (IsGlobalFlagEnabled()) {
    GTEST_SKIP() << "Only applies when the side panel is not global.";
  }
  ASSERT_FALSE(ai_chat::ShouldSidePanelBeGlobal(browser()->GetProfile()));

  const std::string uuid = CreateConversation(browser());
  ASSERT_FALSE(uuid.empty());
  ai_chat::OpenConversationInSidePanel(browser()->GetProfile(), uuid);

  EXPECT_FALSE(IsSidePanelOpen(browser()));
  EXPECT_FALSE(ai_chat::GetSidePanelWebContents(browser()));
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

// Links followed from the side panel must reach the linked site (B) as
// cross-site navigations initiated by AI Chat, not as browser-initiated ones
// (which would send SameSite=Strict cookies and `Sec-Fetch-Site: none`).
// Mirrors `SplitViewLinkTest.CrossSiteTargetBlankLinkRedirectKeepsInitiator`.
class AIChatSidePanelLinkBrowserTest : public AIChatGlobalSidePanelBrowserTest {
 public:
  void SetUpOnMainThread() override {
    AIChatGlobalSidePanelBrowserTest::SetUpOnMainThread();
    service_ = ai_chat::AIChatServiceFactory::GetForBrowserContext(
        browser()->GetProfile());
    ASSERT_TRUE(service_);

    host_resolver()->AddRule("*", "127.0.0.1");
    // Sec-Fetch-* headers are only sent over HTTPS.
    ASSERT_TRUE(embedded_https_test_server().Start());
  }

 protected:
  struct EchoedHeaders {
    std::string cookie;
    std::string sec_fetch_site;
  };

  // Site A: the tab next to the side panel.
  GURL GetSiteAURL() {
    return embedded_https_test_server().GetURL("a.com", "/title1.html");
  }

  // Site B: echoes the `Cookie` and `Sec-Fetch-Site` headers, one per line.
  GURL GetSiteBEchoURL() {
    return embedded_https_test_server().GetURL(
        "b.com", "/echoheader?Cookie&Sec-Fetch-Site");
  }

  void SetSameSiteCookies(const GURL& url) {
    ASSERT_TRUE(
        content::SetCookie(browser()->GetProfile(), url,
                           "strict=1; SameSite=Strict; Secure; Path=/"));
    ASSERT_TRUE(content::SetCookie(browser()->GetProfile(), url,
                                   "lax=1; SameSite=Lax; Secure; Path=/"));
    ASSERT_TRUE(content::SetCookie(browser()->GetProfile(), url,
                                   "none=1; SameSite=None; Secure; Path=/"));
  }

  static EchoedHeaders GetEchoedHeaders(content::WebContents* contents) {
    const std::string body =
        content::EvalJs(contents, "document.body.innerText",
                        content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
                        ISOLATED_WORLD_ID_BRAVE_INTERNAL)
            .ExtractString();
    const std::vector<std::string> lines = base::SplitString(
        body, "\n", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
    EXPECT_EQ(lines.size(), 2u) << body;
    if (lines.size() != 2u) {
      return {};
    }
    return {lines[0], lines[1]};
  }

  // Shows a mock-engine conversation in the side panel next to site A and
  // returns its conversation entries frame.
  content::RenderFrameHost* ShowConversationInSidePanel() {
    // Open site A first, since a non-global panel belongs to the active tab.
    EXPECT_TRUE(ui_test_utils::NavigateToURL(browser(), GetSiteAURL()));

    auto* coordinator = SidePanelCoordinator::From(browser());
    EXPECT_TRUE(coordinator);
    if (!coordinator) {
      return nullptr;
    }
    coordinator->Show(SidePanelEntry::Id::kChatUI);

    content::WebContents* panel_contents = nullptr;
    EXPECT_TRUE(base::test::RunUntil([&]() {
      panel_contents = ai_chat::GetSidePanelWebContents(browser());
      return IsSidePanelOpen(browser()) && panel_contents != nullptr &&
             panel_contents->GetWebUI() != nullptr &&
             !panel_contents->IsLoading();
    }));
    if (!panel_contents) {
      return nullptr;
    }

    CreateConversationWithMockEngine();
    EXPECT_TRUE(content::NavigateToURL(
        panel_contents, ai_chat::ConversationUrl(
                            conversation_handler_->get_conversation_uuid())));

    content::RenderFrameHost* entries_frame = nullptr;
    EXPECT_TRUE(base::test::RunUntil([&]() {
      panel_contents->ForEachRenderFrameHost(
          [&](content::RenderFrameHost* frame) {
            if (frame->GetWebUI() &&
                frame->GetWebUI()
                    ->GetController()
                    ->GetAs<AIChatUntrustedConversationUI>()) {
              entries_frame = frame;
            }
          });
      return entries_frame != nullptr;
    }));
    return entries_frame;
  }

  // Has the mock engine answer with `completion`, citing `sources`.
  void Respond(const std::string& completion, base::span<const GURL> sources) {
    auto generate_future = SetupMockGenerateAssistantResponse();
    conversation_handler_->SubmitHumanConversationEntry("Question",
                                                        std::nullopt);
    auto callbacks = generate_future->Take();

    if (!sources.empty()) {
      std::vector<ai_chat::mojom::WebSourcePtr> web_sources;
      for (const GURL& url : sources) {
        web_sources.push_back(ai_chat::mojom::WebSource::New(
            "Source", url, GURL(), std::nullopt, std::nullopt));
      }
      callbacks.data_callback.Run(ai_chat::EngineConsumer::GenerationResultData(
          ai_chat::mojom::ConversationEntryEvent::NewSourcesEvent(
              ai_chat::mojom::WebSourcesEvent::New(std::move(web_sources),
                                                   std::vector<std::string>())),
          std::nullopt));
    }
    callbacks.data_callback.Run(ai_chat::EngineConsumer::GenerationResultData(
        ai_chat::mojom::ConversationEntryEvent::NewCompletionEvent(
            ai_chat::mojom::CompletionEvent::New(completion)),
        std::nullopt));
    std::move(callbacks.completed_callback)
        .Run(base::ok(ai_chat::EngineConsumer::GenerationResultData(
            nullptr, std::nullopt)));
  }

  // Waits for the response in `frame` to render a link with `text` to `url`.
  bool WaitForLink(content::RenderFrameHost* frame,
                   const GURL& url,
                   std::string_view text,
                   base::Location location = base::Location::Current()) {
    SCOPED_TRACE(location.ToString());
    // Resolves once the link exists, now or after a later DOM change.
    constexpr char kWaitForLinkScript[] = R"(
      new Promise((resolve) => {
        const hasLink = () =>
            [...document.querySelectorAll('a[target=_blank]')].some(
                (a) => a.href === $1 && a.textContent.trim() === $2);
        if (hasLink()) {
          resolve(true);
          return;
        }
        const observer = new MutationObserver(() => {
          if (hasLink()) {
            observer.disconnect();
            resolve(true);
          }
        });
        // `document`, as the body may not exist yet.
        observer.observe(document,
            {childList: true, subtree: true, characterData: true,
             attributes: true});
      })
    )";
    const content::EvalJsResult result = content::EvalJs(
        frame, content::JsReplace(kWaitForLinkScript, url, text),
        content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
        ISOLATED_WORLD_ID_BRAVE_INTERNAL);
    EXPECT_EQ(true, result);
    return result == true;
  }

  // Clicks the link and returns the tab it loaded in.
  content::WebContents* FollowLink(
      content::RenderFrameHost* frame,
      const GURL& url,
      std::string_view text,
      base::Location location = base::Location::Current()) {
    SCOPED_TRACE(location.ToString());
    constexpr char kClickLinkScript[] = R"(
      [...document.querySelectorAll('a[target=_blank]')]
          .find((a) => a.href === $1 && a.textContent.trim() === $2)
          .click()
    )";
    content::TestNavigationObserver observer(url);
    observer.StartWatchingNewWebContents();
    observer.WatchExistingWebContents();
    EXPECT_TRUE(content::ExecJs(frame,
                                content::JsReplace(kClickLinkScript, url, text),
                                content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
                                ISOLATED_WORLD_ID_BRAVE_INTERNAL));
    observer.Wait();
    content::WebContents* active =
        browser()->tab_strip_model()->GetActiveWebContents();
    EXPECT_EQ(active->GetLastCommittedURL(), url);
    return active;
  }

  // Checks where the link opened and that B saw a cross-site request.
  void ExpectCrossSiteNavigation(
      content::WebContents* site_a_tab,
      int initial_tab_count,
      content::WebContents* opened,
      base::Location location = base::Location::Current()) {
    SCOPED_TRACE(location.ToString());
    auto* tab_strip = browser()->tab_strip_model();
    if (IsGlobalFlagEnabled()) {
      // The tab next to the panel navigated from A to B.
      EXPECT_EQ(opened, site_a_tab);
      EXPECT_EQ(tab_strip->count(), initial_tab_count);
    } else {
      EXPECT_NE(opened, site_a_tab);
      EXPECT_EQ(tab_strip->count(), initial_tab_count + 1);
    }

    const EchoedHeaders headers = GetEchoedHeaders(opened);
    const std::string context =
        base::StrCat({"Cookie: ", headers.cookie,
                      ", Sec-Fetch-Site: ", headers.sec_fetch_site,
                      ", URL: ", opened->GetLastCommittedURL().spec()});
    EXPECT_EQ(headers.sec_fetch_site, "cross-site") << context;
    EXPECT_THAT(headers.cookie, testing::Not(testing::HasSubstr("strict=1")))
        << context;
    EXPECT_THAT(headers.cookie, testing::HasSubstr("lax=1")) << context;
    EXPECT_THAT(headers.cookie, testing::HasSubstr("none=1")) << context;
  }
};

// Control: a typed navigation to B does send the Strict cookie.
IN_PROC_BROWSER_TEST_P(AIChatSidePanelLinkBrowserTest,
                       TypedNavigationSendsStrictCookie) {
  const GURL echo_url = GetSiteBEchoURL();
  SetSameSiteCookies(echo_url);

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), echo_url));
  const EchoedHeaders headers =
      GetEchoedHeaders(browser()->tab_strip_model()->GetActiveWebContents());
  EXPECT_EQ(headers.sec_fetch_site, "none") << "Cookie: " << headers.cookie;
  EXPECT_THAT(headers.cookie, testing::HasSubstr("strict=1"));
  EXPECT_THAT(headers.cookie, testing::HasSubstr("lax=1"));
  EXPECT_THAT(headers.cookie, testing::HasSubstr("none=1"));
}

IN_PROC_BROWSER_TEST_P(AIChatSidePanelLinkBrowserTest,
                       CrossSiteAnswerLinkKeepsInitiator) {
  const GURL echo_url = GetSiteBEchoURL();
  SetSameSiteCookies(echo_url);

  content::RenderFrameHost* entries_frame = ShowConversationInSidePanel();
  ASSERT_TRUE(entries_frame);
  content::WebContents* site_a_tab =
      browser()->tab_strip_model()->GetActiveWebContents();
  const int initial_tab_count = browser()->tab_strip_model()->count();

  constexpr char kLinkText[] = "Answer link";
  Respond(base::StrCat({"See [", kLinkText, "](", echo_url.spec(), ")."}),
          /*sources=*/{});
  ASSERT_TRUE(WaitForLink(entries_frame, echo_url, kLinkText));

  ExpectCrossSiteNavigation(site_a_tab, initial_tab_count,
                            FollowLink(entries_frame, echo_url, kLinkText));
}

IN_PROC_BROWSER_TEST_P(AIChatSidePanelLinkBrowserTest,
                       CrossSiteCitationLinkKeepsInitiator) {
  const GURL echo_url = GetSiteBEchoURL();
  SetSameSiteCookies(echo_url);

  content::RenderFrameHost* entries_frame = ShowConversationInSidePanel();
  ASSERT_TRUE(entries_frame);
  content::WebContents* site_a_tab =
      browser()->tab_strip_model()->GetActiveWebContents();
  const int initial_tab_count = browser()->tab_strip_model()->count();

  Respond("A cited claim [1].", /*sources=*/{echo_url});
  ASSERT_TRUE(WaitForLink(entries_frame, echo_url, "1"));

  ExpectCrossSiteNavigation(site_a_tab, initial_tab_count,
                            FollowLink(entries_frame, echo_url, "1"));
}

INSTANTIATE_TEST_SUITE_P(
    ,
    AIChatSidePanelLinkBrowserTest,
    testing::Combine(testing::Bool(), testing::Bool()),
    [](const testing::TestParamInfo<AIChatSidePanelLinkBrowserTest::ParamType>&
           info) {
      return absl::StrFormat(
          "GlobalSidePanel_%s_MoveToSidePanel_%s",
          std::get<0>(info.param) ? "Enabled" : "NotEnabled",
          std::get<1>(info.param) ? "Enabled" : "NotEnabled");
    });
