// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/webui/tab_search/tab_search_page_handler.h"

#include <memory>
#include <string>
#include <string_view>

#include "base/memory/raw_ptr.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/bind.h"
#include "base/test/gmock_callback_support.h"
#include "base/test/mock_callback.h"
#include "brave/components/ai_chat/core/browser/engine/mock_engine_consumer.h"
#include "brave/components/ai_chat/core/browser/types.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profile_test_util.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/test/test_browser_closed_waiter.h"
#include "chrome/browser/ui/webui/tab_search/tab_search_ui.h"
#include "chrome/common/webui_url_constants.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/grit/brave_components_strings.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_utils.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/gurl.h"

namespace {

constexpr char kFooDotComUrl1[] = "https://foo.com/1";
constexpr char kFooDotComUrl2[] = "https://foo.com/2";
constexpr char kBarDotComUrl1[] = "https://bar.com/1";
constexpr char kBarDotComUrl2[] = "https://bar.com/2";
constexpr char kCatDotComUrl1[] = "https://cat.com/1";
constexpr char kCatDotComUrl2[] = "https://cat.com/2";
constexpr char kDogDotComUrl1[] = "https://dog.com/1";

constexpr char kFooDotComOrigin[] = "https://foo.com";
constexpr char kBarDotComOrigin[] = "https://bar.com";

constexpr char kFooDotComTitle1[] = "foo.com 1";
constexpr char kFooDotComTitle2[] = "foo.com 2";
constexpr char kBarDotComTitle1[] = "bar.com 1";
constexpr char kBarDotComTitle2[] = "bar.com 2";
constexpr char kCatDotComTitle1[] = "cat.com 1";
constexpr char kCatDotComTitle2[] = "cat.com 2";
constexpr char kDogDotComTitle1[] = "dog.com 1";

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

    // Create another profile.
    ProfileManager* profile_manager = g_browser_process->profile_manager();
    profile2_ = &profiles::testing::CreateProfileSync(
        profile_manager, profile_manager->GenerateNextProfileDirectoryPath());

    // Create a few browser windows.
    // browser1: normal window with default profile. (already created)
    // browser2: normal window with defauit profile.
    ui_test_utils::OpenNewEmptyWindowAndWaitUntilActivated(profile1());
    browser2_ = BrowserList::GetInstance()->GetLastActive();
    // browser3: private window.
    ui_test_utils::OpenNewEmptyWindowAndWaitUntilActivated(
        profile1()->GetPrimaryOTRProfile(true));
    browser3_ = BrowserList::GetInstance()->GetLastActive();
    // browser4: normal window with profile2.
    ui_test_utils::OpenNewEmptyWindowAndWaitUntilActivated(profile2());
    browser4_ = BrowserList::GetInstance()->GetLastActive();
    // browser5: non-normal window with default profile. new custom CreateParam.
    Browser::CreateParams params =
        Browser::CreateParams(Browser::TYPE_POPUP, profile1(), true);
    browser5_ = Browser::Create(params);
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

  Profile* profile2() { return profile2_; }

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

  // browser1 is a normal window with default profile.
  Browser* browser1() { return browser(); }

  // browser2 is a normal window with default profile.
  Browser* browser2() { return browser2_; }

  // browser3 is a private window.
  Browser* browser3() { return browser3_; }

  // browser4 is a normal window with profile2.
  Browser* browser4() { return browser4_; }

  // browser5 is a non-normal window with default profile.
  Browser* browser5() { return browser5_; }

 protected:
  std::unique_ptr<content::WebContents> webui_contents_;

  raw_ptr<Profile> profile2_;
  raw_ptr<Browser> browser2_;
  raw_ptr<Browser> browser3_;
  raw_ptr<Browser> browser4_;
  raw_ptr<Browser> browser5_;
};

IN_PROC_BROWSER_TEST_F(TabSearchPageHandlerBrowserTest, GetFocusTabs) {
  // Test Engine's GetFocusTabs is called with expected tabs info and topic.
  // Test OnGetFocusTabs:
  // 1. Normal tabs moved to a new window.
  // 2. Unexisting Tab ID.
  // 3. Incognito tab won't be moved even the ID is somehow returned.
  // 4. Tab from other profiles won't be moved even the ID is somehow returned.
  // 5. Tab from non-normal window won't be moved even the ID is somehow
  // returned.
  // 6. Undo info stored as expected.
  handler()->SetAIChatEngineForTesting(
      std::make_unique<testing::NiceMock<ai_chat::MockEngineConsumer>>());

  // Add tabs in windows with the default profile.
  AppendTabWithTitle(browser1(), GURL(kFooDotComUrl1), kFooDotComTitle1);
  AppendTabWithTitle(browser1(), GURL(kFooDotComUrl2), kFooDotComTitle2);
  AppendTabWithTitle(browser2(), GURL(kBarDotComUrl1), kBarDotComTitle1);
  AppendTabWithTitle(browser2(), GURL(kBarDotComUrl2), kBarDotComTitle2);
  // Add a tab in a private window.
  AppendTabWithTitle(browser3(), GURL(kCatDotComUrl1), kCatDotComTitle1);
  // Add a tab in a window with different profile.
  AppendTabWithTitle(browser4(), GURL(kCatDotComUrl2), kCatDotComTitle2);
  // Add a tab in a non-normal window.
  AppendTabWithTitle(browser5(), GURL(kDogDotComUrl1), kDogDotComTitle1);

  ASSERT_EQ(browser1()->tab_strip_model()->count(), 3);
  ASSERT_EQ(browser2()->tab_strip_model()->count(), 3);
  ASSERT_EQ(browser3()->tab_strip_model()->count(), 2);
  ASSERT_EQ(browser4()->tab_strip_model()->count(), 2);
  ASSERT_EQ(browser5()->tab_strip_model()->count(), 1);

  const int tab_id1 =
      browser1()->tab_strip_model()->GetTabAtIndex(1)->GetHandle().raw_value();
  const int tab_id2 =
      browser1()->tab_strip_model()->GetTabAtIndex(2)->GetHandle().raw_value();
  const int tab_id3 =
      browser2()->tab_strip_model()->GetTabAtIndex(1)->GetHandle().raw_value();
  const int tab_id4 =
      browser2()->tab_strip_model()->GetTabAtIndex(2)->GetHandle().raw_value();
  const int tab_id5 =
      browser3()->tab_strip_model()->GetTabAtIndex(1)->GetHandle().raw_value();
  const int tab_id6 =
      browser4()->tab_strip_model()->GetTabAtIndex(1)->GetHandle().raw_value();
  const int tab_id7 =
      browser5()->tab_strip_model()->GetTabAtIndex(0)->GetHandle().raw_value();

  // Only tab from the same regular profile and normal window would be used.
  std::vector<ai_chat::Tab> expected_tabs = {
      {base::NumberToString(tab_id1), kFooDotComTitle1, kFooDotComOrigin},
      {base::NumberToString(tab_id2), kFooDotComTitle2, kFooDotComOrigin},
      {base::NumberToString(tab_id3), kBarDotComTitle1, kBarDotComOrigin},
      {base::NumberToString(tab_id4), kBarDotComTitle2, kBarDotComOrigin},
  };

  // Mock return tabs with normal case (tab_id1, tab_id4), unexisting tab
  // ("100", "invalid"), incognito tab (tab_id5), tab from other profile
  // (tab_id6), tab from non-normal window (tab_id7). Only tab1 and tab4 should
  // be moved to a new window.
  std::vector<std::string> mock_ret_tabs = {base::NumberToString(tab_id1),
                                            base::NumberToString(tab_id4),
                                            "100",
                                            "invalid",
                                            base::NumberToString(tab_id5),
                                            base::NumberToString(tab_id6),
                                            base::NumberToString(tab_id7)};
  auto* mock_engine = static_cast<ai_chat::MockEngineConsumer*>(
      handler()->GetAIChatEngineForTesting());
  EXPECT_CALL(*mock_engine, GetFocusTabs(expected_tabs, "topic", _))
      .WillOnce(base::test::RunOnceCallback<2>(mock_ret_tabs));

  handler()->GetFocusTabs("topic", base::BindLambdaForTesting(
                                       [&](bool new_window_created,
                                           tab_search::mojom::ErrorPtr error) {
                                         EXPECT_TRUE(new_window_created);
                                         EXPECT_FALSE(error);
                                       }));

  BrowserList* browser_list = BrowserList::GetInstance();
  Browser* active_browser = browser_list->GetLastActive();
  ASSERT_EQ(browser_list->size(), 6u) << "A new window should be created.";
  ASSERT_EQ(active_browser, browser_list->get(5))
      << "The new window should be active.";
  EXPECT_EQ(active_browser->tab_strip_model()->count(), 2)
      << "The new window should have 2 tabs.";
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
  handler()->UndoFocusTabs(base::BindLambdaForTesting([&]() {
    // Wait for the new window to be closed.
    ASSERT_EQ(browser_list->size(), 6u);
    ASSERT_TRUE(
        TestBrowserClosedWaiter(browser_list->get(5)).WaitUntilClosed());

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
  }));

  testing::Mock::VerifyAndClearExpectations(mock_engine);
  EXPECT_CALL(*mock_engine, GetFocusTabs(expected_tabs, "topic", _))
      .WillOnce(base::test::RunOnceCallback<2>(
          base::unexpected(ai_chat::mojom::APIError::RateLimitReached)));

  // Test error.
  handler()->GetFocusTabs(
      "topic",
      base::BindLambdaForTesting(
          [&](bool new_window_created, tab_search::mojom::ErrorPtr error) {
            EXPECT_FALSE(new_window_created);
            auto rate_limited_info =
                tab_search::mojom::RateLimitedInfo::New(false /* is_premium */);
            EXPECT_EQ(error, tab_search::mojom::Error::New(
                                 l10n_util::GetStringUTF8(
                                     IDS_CHAT_UI_RATE_LIMIT_REACHED_DESC),
                                 std::move(rate_limited_info)));
          }));
}

IN_PROC_BROWSER_TEST_F(TabSearchPageHandlerBrowserTest, UndoFocusTabs) {
  // Add tabs in windows with the default profile.
  AppendTabWithTitle(browser1(), GURL(kFooDotComUrl1), kFooDotComTitle1);
  AppendTabWithTitle(browser1(), GURL(kFooDotComUrl2), kFooDotComTitle2);
  AppendTabWithTitle(browser2(), GURL(kBarDotComUrl1), kBarDotComTitle1);
  AppendTabWithTitle(browser2(), GURL(kBarDotComUrl2), kBarDotComTitle2);
  AppendTabWithTitle(browser2(), GURL(kCatDotComUrl1), kCatDotComTitle1);
  AppendTabWithTitle(browser2(), GURL(kCatDotComUrl2), kCatDotComTitle2);
  // Close the blank tabs when creating a new window.
  browser1()->tab_strip_model()->CloseWebContentsAt(0,
                                                    TabCloseTypes::CLOSE_NONE);
  browser2()->tab_strip_model()->CloseWebContentsAt(0,
                                                    TabCloseTypes::CLOSE_NONE);

  ASSERT_EQ(browser1()->tab_strip_model()->count(), 2);
  ASSERT_EQ(browser2()->tab_strip_model()->count(), 4);

  const int tab_id1 =
      browser1()->tab_strip_model()->GetTabAtIndex(0)->GetHandle().raw_value();
  const int tab_id2 =
      browser1()->tab_strip_model()->GetTabAtIndex(1)->GetHandle().raw_value();
  const int tab_id3 =
      browser2()->tab_strip_model()->GetTabAtIndex(0)->GetHandle().raw_value();
  const int tab_id4 =
      browser2()->tab_strip_model()->GetTabAtIndex(1)->GetHandle().raw_value();
  const int tab_id5 =
      browser2()->tab_strip_model()->GetTabAtIndex(2)->GetHandle().raw_value();
  const int tab_id6 =
      browser2()->tab_strip_model()->GetTabAtIndex(3)->GetHandle().raw_value();

  // Close tab_id6 to have a non-existing tab ID inside, for example, tab is
  // closed in the new window.
  browser2()->tab_strip_model()->CloseWebContentsAt(3,
                                                    TabCloseTypes::CLOSE_NONE);

  base::flat_map<SessionID, std::vector<TabSearchPageHandler::TabInfo>>
      undo_info;
  // Use browser1's session ID to mock that these tabs were moved from browser1.
  undo_info[browser1()->session_id()] = {
      {tab_id3, 2},
      {tab_id4, 1},
      // Index 5 is bigger than the last index after restored, this can happen
      // when a tab in the original window is closed before undo.
      {tab_id5, 5},
      {tab_id6, 6},
      {100, 5},
  };
  handler()->SetFocusTabsInfoForTesting(undo_info);
  handler()->UndoFocusTabs(base::BindLambdaForTesting([&]() {
    BrowserList* browser_list = BrowserList::GetInstance();
    // Wait for the new window to be closed.
    ASSERT_TRUE(
        TestBrowserClosedWaiter(browser_list->get(1)).WaitUntilClosed());
    Browser* browser1 = browser_list->get(0);
    EXPECT_EQ(browser1->tab_strip_model()->count(), 5)
        << "The tabs should be moved back to the window stored.";
    EXPECT_EQ(
        browser1->tab_strip_model()->GetTabAtIndex(0)->GetHandle().raw_value(),
        tab_id1);
    EXPECT_EQ(
        browser1->tab_strip_model()->GetTabAtIndex(1)->GetHandle().raw_value(),
        tab_id4);
    EXPECT_EQ(
        browser1->tab_strip_model()->GetTabAtIndex(2)->GetHandle().raw_value(),
        tab_id3);
    EXPECT_EQ(
        browser1->tab_strip_model()->GetTabAtIndex(3)->GetHandle().raw_value(),
        tab_id2);
    EXPECT_EQ(
        browser1->tab_strip_model()->GetTabAtIndex(4)->GetHandle().raw_value(),
        tab_id5);
  }));
}
