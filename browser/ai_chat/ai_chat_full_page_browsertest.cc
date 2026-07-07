// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <vector>

#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/ai_chat/ai_chat_conversation_ui_browsertest_base.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/side_panel/side_panel_entry_id.h"
#include "chrome/browser/ui/side_panel/side_panel_ui.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/side_panel/side_panel.h"
#include "chrome/browser/ui/views/side_panel/side_panel_web_ui_view.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/url_constants.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/views/controls/webview/webview.h"
#include "url/gurl.h"

namespace ai_chat {

// Tests the "open in full page" action, which moves a conversation from the AI
// Chat side panel to a full-page tab and closes the (now duplicate) side panel.
// Parameterized over whether the side panel is global (persists across tab
// switches) or per-tab. The global case is what actually exercises the explicit
// close: a per-tab panel would close on its own when the new tab is opened.
class AIChatFullPageBrowserTest : public AIChatConversationUIBrowserTestBase,
                                  public testing::WithParamInterface<bool> {
 public:
  AIChatFullPageBrowserTest() {
    // The full page action and its button both require the history feature.
    std::vector<base::test::FeatureRef> enabled_features = {
        features::kAIChatHistory};
    std::vector<base::test::FeatureRef> disabled_features;
    if (IsGlobalSidePanel()) {
      enabled_features.push_back(features::kAIChatGlobalSidePanelEverywhere);
    } else {
      disabled_features.push_back(features::kAIChatGlobalSidePanelEverywhere);
    }
    scoped_feature_list_.InitWithFeatures(enabled_features, disabled_features);
  }
  ~AIChatFullPageBrowserTest() override = default;

  bool IsGlobalSidePanel() const { return GetParam(); }

 protected:
  SidePanelUI* side_panel_ui() {
    return browser()->GetFeatures().side_panel_ui();
  }

  bool IsChatSidePanelShowing() {
    return side_panel_ui()->IsSidePanelShowing() &&
           side_panel_ui()->GetCurrentEntryId() == SidePanelEntryId::kChatUI;
  }

  // Opens the AI Chat side panel and returns its (trusted) main frame, which
  // hosts the conversation header and its "open in full page" button.
  content::RenderFrameHost* OpenChatSidePanel() {
    side_panel_ui()->Show(SidePanelEntryId::kChatUI);
    auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser());
    EXPECT_TRUE(browser_view);
    auto* side_panel_view = browser_view->side_panel()->GetViewByID(
        SidePanelWebUIView::kSidePanelWebViewId);
    EXPECT_TRUE(side_panel_view);
    auto* side_panel_web_contents =
        static_cast<views::WebView*>(side_panel_view)->web_contents();
    EXPECT_TRUE(side_panel_web_contents);
    EXPECT_TRUE(content::WaitForLoadStop(side_panel_web_contents));
    return side_panel_web_contents->GetPrimaryMainFrame();
  }

  base::test::ScopedFeatureList scoped_feature_list_;
};

// Clicking the "open in full page" button in the side panel should open the
// conversation in a new full-page tab and close the AI Chat side panel, so the
// same conversation isn't shown in two places at once.
IN_PROC_BROWSER_TEST_P(AIChatFullPageBrowserTest,
                       OpenConversationFullPageClosesChatSidePanel) {
  content::RenderFrameHost* side_panel_frame = OpenChatSidePanel();
  ASSERT_TRUE(side_panel_frame);
  ASSERT_TRUE(IsChatSidePanelShowing());

  const int initial_tab_count = browser()->tab_strip_model()->count();

  // The button only renders once the conversation is bound and has a uuid, so
  // wait for it before clicking. Clicking invokes OpenConversationFullPage().
  ASSERT_TRUE(VerifyElementState("open-full-page-button", /*expect_exist=*/true,
                                 side_panel_frame));
  ASSERT_TRUE(ClickElement("open-full-page-button", side_panel_frame));

  // A new foreground tab should open at the conversation's full-page URL.
  ASSERT_TRUE(base::test::RunUntil([&]() {
    return browser()->tab_strip_model()->count() == initial_tab_count + 1;
  }));
  content::WebContents* new_tab =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(content::WaitForLoadStop(new_tab));
  const GURL& full_page_url = new_tab->GetLastCommittedURL();
  EXPECT_TRUE(full_page_url.SchemeIs(content::kChromeUIScheme));
  EXPECT_EQ(full_page_url.host(), "leo-ai");
  // The path carries the conversation uuid, e.g. chrome://leo-ai/<uuid>.
  EXPECT_GT(full_page_url.path().length(), 1u);

  // The AI Chat side panel should now be closed, since the conversation moved
  // to a full-page tab.
  ASSERT_TRUE(
      base::test::RunUntil([&]() { return !IsChatSidePanelShowing(); }));
}

INSTANTIATE_TEST_SUITE_P(
    ,
    AIChatFullPageBrowserTest,
    testing::Bool(),
    [](const testing::TestParamInfo<AIChatFullPageBrowserTest::ParamType>&
           info) {
      return info.param ? "GlobalSidePanel" : "PerTabSidePanel";
    });

}  // namespace ai_chat
