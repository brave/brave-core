// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/webui/tab_search/tab_search_page_handler.h"

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/bind.h"
#include "base/test/gmock_callback_support.h"
#include "base/test/mock_callback.h"
#include "brave/browser/ai_chat/ai_chat_service_factory.h"
#include "brave/components/ai_chat/core/browser/ai_chat_service.h"
#include "brave/components/ai_chat/core/browser/constants.h"
#include "brave/components/ai_chat/core/browser/engine/mock_engine_consumer.h"
#include "brave/components/ai_chat/core/browser/types.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/test/test_browser_closed_waiter.h"
#include "chrome/browser/ui/webui/tab_search/tab_search_ui.h"
#include "chrome/common/webui_url_constants.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/grit/brave_components_strings.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_utils.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace {

constexpr char kFooDotComUrl1[] = "https://foo.com/1";
constexpr char kFooDotComUrl2[] = "https://foo.com/2";
constexpr char kBarDotComUrl1[] = "https://bar.com/1";
constexpr char kBarDotComUrl2[] = "https://bar.com/2";

constexpr char kFooDotComTitle1[] = "foo.com 1";
constexpr char kFooDotComTitle2[] = "foo.com 2";
constexpr char kBarDotComTitle1[] = "bar.com 1";
constexpr char kBarDotComTitle2[] = "bar.com 2";

}  // namespace

using testing::_;

class TabSearchPageHandlerBrowserTest : public InProcessBrowserTest {
 public:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    webui_contents_ = content::WebContents::Create(
        content::WebContents::CreateParams(browser()->profile()));

    webui_contents_->GetController().LoadURLWithParams(
        content::NavigationController::LoadURLParams(
            GURL(chrome::kChromeUITabSearchURL)));

    // Finish loading after initializing.
    ASSERT_TRUE(content::WaitForLoadStop(webui_contents_.get()));

    // Create another browser with the default profile.
    chrome::NewEmptyWindow(profile1(), false);
  }

  void TearDownOnMainThread() override {
    webui_contents_.reset();
    InProcessBrowserTest::TearDownOnMainThread();
  }

  TabSearchPageHandler* handler() {
    return webui_contents_->GetWebUI()
        ->GetController()
        ->template GetAs<TabSearchUI>()
        ->page_handler_for_testing();
  }

  Profile* profile1() { return browser()->profile(); }

  void AppendTabWithTitle(Browser* browser,
                          const GURL& url,
                          const std::string& title) {
    chrome::AddTabAt(browser, url, -1, true);
    content::WebContents* web_contents =
        browser->tab_strip_model()->GetActiveWebContents();
    web_contents->UpdateTitleForEntry(
        web_contents->GetController().GetLastCommittedEntry(),
        base::UTF8ToUTF16(title));
  }

 protected:
  std::unique_ptr<content::WebContents> webui_contents_;
};

IN_PROC_BROWSER_TEST_F(TabSearchPageHandlerBrowserTest, GetFocusTabs) {
  // Test Engine's GetFocusTabs is called with expected tabs info and topic.
  auto* ai_chat_service =
      ai_chat::AIChatServiceFactory::GetForBrowserContext(profile1());
  ai_chat_service->SetTabOrganizationEngineForTesting(
      std::make_unique<ai_chat::MockEngineConsumer>());

  BrowserList* browser_list = BrowserList::GetInstance();
  ASSERT_EQ(browser_list->size(), 2u);
  Browser* browser2 = browser_list->get(1);

  // Add tabs in windows with the default profile.
  AppendTabWithTitle(browser(), GURL(kFooDotComUrl1), kFooDotComTitle1);
  AppendTabWithTitle(browser(), GURL(kFooDotComUrl2), kFooDotComTitle2);
  AppendTabWithTitle(browser2, GURL(kBarDotComUrl1), kBarDotComTitle1);
  AppendTabWithTitle(browser2, GURL(kBarDotComUrl2), kBarDotComTitle2);

  // size is 3 because a blank tab is added by default when creating windows.
  ASSERT_EQ(browser()->tab_strip_model()->count(), 3);
  ASSERT_EQ(browser2->tab_strip_model()->count(), 3);

  const int tab_id1 =
      browser()->tab_strip_model()->GetTabAtIndex(1)->GetHandle().raw_value();
  const int tab_id2 =
      browser()->tab_strip_model()->GetTabAtIndex(2)->GetHandle().raw_value();
  const int tab_id3 =
      browser2->tab_strip_model()->GetTabAtIndex(1)->GetHandle().raw_value();
  const int tab_id4 =
      browser2->tab_strip_model()->GetTabAtIndex(2)->GetHandle().raw_value();

  std::vector<ai_chat::Tab> expected_tabs = {
      {base::NumberToString(tab_id1), kFooDotComTitle1,
       url::Origin::Create(GURL(kFooDotComUrl1))},
      {base::NumberToString(tab_id2), kFooDotComTitle2,
       url::Origin::Create(GURL(kFooDotComUrl2))},
      {base::NumberToString(tab_id3), kBarDotComTitle1,
       url::Origin::Create(GURL(kBarDotComUrl1))},
      {base::NumberToString(tab_id4), kBarDotComTitle2,
       url::Origin::Create(GURL(kBarDotComUrl2))},
  };

  std::vector<std::string> mock_ret_tabs = {base::NumberToString(tab_id1),
                                            "100", "invalid",
                                            base::NumberToString(tab_id4)};
  auto* mock_engine = static_cast<ai_chat::MockEngineConsumer*>(
      ai_chat_service->GetTabOrganizationEngineForTesting());
  std::string model_name = ai_chat::kClaudeHaikuModelName;
  EXPECT_CALL(*mock_engine, GetModelName())
      .WillOnce(testing::ReturnRef(model_name));
  EXPECT_CALL(*mock_engine, GetFocusTabs(expected_tabs, "topic", _))
      .WillOnce(base::test::RunOnceCallback<2>(mock_ret_tabs));

  base::RunLoop run_loop1;
  handler()->GetFocusTabs("topic", base::BindLambdaForTesting(
                                       [&](bool new_window_created,
                                           tab_search::mojom::ErrorPtr error) {
                                         EXPECT_TRUE(new_window_created);
                                         EXPECT_FALSE(error);
                                         run_loop1.Quit();
                                       }));
  run_loop1.Run();

  Browser* active_browser = browser_list->GetLastActive();
  ASSERT_EQ(browser_list->size(), 3u) << "A new window should be created.";
  ASSERT_EQ(active_browser, browser_list->get(2))
      << "The new window should be active.";
  EXPECT_EQ(active_browser->tab_strip_model()->count(), 2)
      << "The new window should have 2 tabs.";
  EXPECT_EQ(active_browser->user_title(), "topic");

  // Check the tabs are moved to the new window as expected.
  EXPECT_EQ(active_browser->tab_strip_model()
                ->GetTabAtIndex(0)
                ->GetHandle()
                .raw_value(),
            tab_id1);
  EXPECT_EQ(active_browser->tab_strip_model()
                ->GetTabAtIndex(1)
                ->GetHandle()
                .raw_value(),
            tab_id4);

  // Test undo.
  base::RunLoop run_loop2;
  handler()->UndoFocusTabs(base::BindLambdaForTesting([&]() {
    // Wait for the new window to be closed.
    ASSERT_EQ(browser_list->size(), 3u);
    ASSERT_TRUE(
        TestBrowserClosedWaiter(browser_list->get(2)).WaitUntilClosed());

    Browser* browser1 = browser_list->get(0);
    EXPECT_EQ(browser1->tab_strip_model()->count(), 3)
        << "The tabs should be moved back to the previous active window.";
    EXPECT_EQ(
        browser1->tab_strip_model()->GetTabAtIndex(1)->GetHandle().raw_value(),
        tab_id1);
    Browser* browser2 = browser_list->get(1);
    EXPECT_EQ(browser2->tab_strip_model()->count(), 3)
        << "The tabs should be moved back to the previous active window.";
    EXPECT_EQ(
        browser2->tab_strip_model()->GetTabAtIndex(2)->GetHandle().raw_value(),
        tab_id4);
    run_loop2.Quit();
  }));

  run_loop2.Run();
}
