// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/webui/tab_search/tab_search_page_handler.h"

#include <memory>
#include <string>
#include <vector>

#include "base/functional/callback.h"
#include "base/location.h"
#include "base/memory/raw_ptr.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/cancelable_task_tracker.h"
#include "base/test/bind.h"
#include "base/test/gmock_callback_support.h"
#include "base/test/mock_callback.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/test_future.h"
#include "brave/browser/ai_chat/ai_chat_service_factory.h"
#include "brave/browser/ai_chat/tab_tracker_service_factory.h"
#include "brave/components/ai_chat/core/browser/ai_chat_service.h"
#include "brave/components/ai_chat/core/browser/engine/mock_engine_consumer.h"
#include "brave/components/ai_chat/core/browser/tab_tracker_service.h"
#include "brave/components/ai_chat/core/browser/types.h"
#include "brave/components/ai_chat/core/common/mojom/tab_tracker.mojom.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/history_embeddings/test/fake_history_embeddings_search.h"
#include "brave/components/local_ai/core/pref_names.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface_iterator.h"
#include "chrome/browser/ui/browser_window/public/global_browser_collection.h"
#include "chrome/browser/ui/webui/tab_search/tab_search.mojom-forward.h"
#include "chrome/browser/ui/webui/tab_search/tab_search_ui.h"
#include "chrome/common/webui_url_constants.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/grit/brave_components_strings.h"
#include "components/history/core/browser/history_service.h"
#include "components/history/core/browser/history_types.h"
#include "components/history_embeddings/core/history_embeddings_features.h"
#include "components/keyed_service/core/service_access_type.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "content/public/test/test_utils.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "net/dns/mock_host_resolver.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/page_transition_types.h"
#include "ui/base/window_open_disposition.h"
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

constexpr char kTopic[] = "topic";
constexpr char kTopic2[] = "topic2";

}  // namespace

using testing::_;

class TabChangeWaiter : ai_chat::mojom::TabDataObserver {
 public:
  explicit TabChangeWaiter(content::BrowserContext* context) {
    auto* tracker_service =
        ai_chat::TabTrackerServiceFactory::GetInstance()->GetForBrowserContext(
            context);
    tracker_service->AddObserver(receiver_.BindNewPipeAndPassRemote());
  }

  ~TabChangeWaiter() override = default;

  void WaitForTabDataChanged(const std::vector<std::string>& titles) {
    expected_titles_ = titles;
    base::RunLoop run_loop;
    quit_closure_ = run_loop.QuitClosure();
    run_loop.Run();
  }

  void TabDataChanged(
      std::vector<ai_chat::mojom::TabDataPtr> tab_data) override {
    std::vector<std::string> titles;
    for (const auto& tab : tab_data) {
      titles.push_back(tab->title);
    }

    if (!quit_closure_ || titles != expected_titles_) {
      return;
    }
    std::move(quit_closure_).Run();
  }

 private:
  mojo::Receiver<ai_chat::mojom::TabDataObserver> receiver_{this};
  std::vector<std::string> expected_titles_;
  base::OnceClosure quit_closure_;
};

class TabSearchPageHandlerBrowserTest : public InProcessBrowserTest {
 public:
  TabSearchPageHandlerBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {}

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    base::FilePath test_data_dir =
        base::PathService::CheckedGet(brave::DIR_TEST_DATA);
    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    https_server_.ServeFilesFromDirectory(test_data_dir);
    host_resolver()->AddRule("*", "127.0.0.1");
    EXPECT_TRUE(https_server_.Start());

    webui_contents_ = content::WebContents::Create(
        content::WebContents::CreateParams(browser()->profile()));

    webui_contents_->GetController().LoadURLWithParams(
        content::NavigationController::LoadURLParams(
            GURL(chrome::kChromeUITabSearchURL)));

    // Finish loading after initializing.
    ASSERT_TRUE(content::WaitForLoadStop(webui_contents_.get()));
  }

  void TearDownOnMainThread() override {
    webui_contents_.reset();
    InProcessBrowserTest::TearDownOnMainThread();
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpCommandLine(command_line);
    mock_cert_verifier_.SetUpCommandLine(command_line);
  }

  void SetUpInProcessBrowserTestFixture() override {
    InProcessBrowserTest::SetUpInProcessBrowserTestFixture();
    mock_cert_verifier_.SetUpInProcessBrowserTestFixture();
  }

  void TearDownInProcessBrowserTestFixture() override {
    InProcessBrowserTest::TearDownInProcessBrowserTestFixture();
    mock_cert_verifier_.TearDownInProcessBrowserTestFixture();
  }

  const net::EmbeddedTestServer* https_server() const { return &https_server_; }

  TabSearchPageHandler* handler() {
    return webui_contents_->GetWebUI()
        ->GetController()
        ->template GetAs<TabSearchUI>()
        ->page_handler_for_testing();
  }

  Profile* profile1() { return browser()->profile(); }

  void AppendTabWithTitle(Browser* browser,
                          const GURL& url,
                          const std::string& title,
                          ui_test_utils::BrowserTestWaitFlags wait_flags =
                              ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP) {
    ui_test_utils::NavigateToURLWithDisposition(
        browser, url, WindowOpenDisposition::NEW_FOREGROUND_TAB, wait_flags);
    content::WebContents* web_contents =
        browser->tab_strip_model()->GetActiveWebContents();
    web_contents->UpdateTitleForEntry(
        web_contents->GetController().GetLastCommittedEntry(),
        base::UTF8ToUTF16(title));
  }

  void TestGetSuggestedTopics(const std::vector<std::string>& expected_topics,
                              tab_search::mojom::ErrorPtr expected_error,
                              const base::Location& location = FROM_HERE) {
    SCOPED_TRACE(location.ToString());
    base::RunLoop run_loop;
    handler()->GetSuggestedTopics(
        base::BindLambdaForTesting([&](const std::vector<std::string>& topics,
                                       tab_search::mojom::ErrorPtr error) {
          EXPECT_EQ(topics, expected_topics);
          EXPECT_EQ(error, expected_error);
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  ai_chat::AIChatService* service() {
    return ai_chat::AIChatServiceFactory::GetForBrowserContext(profile1());
  }

  ai_chat::MockEngineConsumer* SetMockTabOrganizationEngine() {
    service()->SetTabOrganizationEngineForTesting(
        std::make_unique<testing::NiceMock<ai_chat::MockEngineConsumer>>());
    auto* mock_engine = static_cast<ai_chat::MockEngineConsumer*>(
        service()->GetTabOrganizationEngineForTesting());
    return mock_engine;
  }

 protected:
  std::unique_ptr<content::WebContents> webui_contents_;

 private:
  net::EmbeddedTestServer https_server_;
  content::ContentMockCertVerifier mock_cert_verifier_;
};

IN_PROC_BROWSER_TEST_F(TabSearchPageHandlerBrowserTest,
                       GetSuggestedTopics_NewTab) {
  auto* mock_engine = SetMockTabOrganizationEngine();
  ASSERT_EQ(mock_engine, service()->GetTabOrganizationEngineForTesting());
  EXPECT_CALL(*mock_engine, GetSuggestedTopics(_, _))
      .WillOnce(
          base::test::RunOnceCallback<1>(std::vector<std::string>{kTopic}))
      .WillOnce(
          base::test::RunOnceCallback<1>(std::vector<std::string>{kTopic2}));

  AppendTabWithTitle(
      browser(), https_server()->GetURL("foo.com", "/simple.html"),
      kFooDotComTitle1, ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);
  TestGetSuggestedTopics({kTopic}, nullptr);

  AppendTabWithTitle(
      browser(), https_server()->GetURL("bar.com", "/simple.html"),
      kBarDotComTitle1, ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);
  TestGetSuggestedTopics({kTopic2}, nullptr);
  // Cached topics should be returned when nothing changed.
  TestGetSuggestedTopics({kTopic2}, nullptr);
}

IN_PROC_BROWSER_TEST_F(TabSearchPageHandlerBrowserTest,
                       GetSuggestedTopics_TitleUpdated) {
  auto* mock_engine = SetMockTabOrganizationEngine();
  EXPECT_CALL(*mock_engine, GetSuggestedTopics(_, _))
      .WillOnce(
          base::test::RunOnceCallback<1>(std::vector<std::string>{kTopic}))
      .WillOnce(
          base::test::RunOnceCallback<1>(std::vector<std::string>{kTopic2}));

  AppendTabWithTitle(
      browser(), https_server()->GetURL("foo.com", "/simple.html"),
      kFooDotComTitle1, ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);
  TestGetSuggestedTopics({kTopic}, nullptr);

  TabChangeWaiter tab_change_waiter(profile1());
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  web_contents->UpdateTitleForEntry(
      web_contents->GetController().GetLastCommittedEntry(), u"New Title");
  tab_change_waiter.WaitForTabDataChanged({"New Title"});
  TestGetSuggestedTopics({kTopic2}, nullptr);
}

IN_PROC_BROWSER_TEST_F(TabSearchPageHandlerBrowserTest,
                       GetSuggestedTopics_TabClosed) {
  auto* mock_engine = SetMockTabOrganizationEngine();
  EXPECT_CALL(*mock_engine, GetSuggestedTopics(_, _))
      .WillOnce(
          base::test::RunOnceCallback<1>(std::vector<std::string>{kTopic}))
      .WillOnce(
          base::test::RunOnceCallback<1>(std::vector<std::string>{kTopic2}));

  AppendTabWithTitle(
      browser(), https_server()->GetURL("foo.com", "/simple.html"),
      kFooDotComTitle1, ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);
  AppendTabWithTitle(
      browser(), https_server()->GetURL("bar.com", "/simple.html"),
      kBarDotComTitle1, ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);
  TestGetSuggestedTopics({kTopic}, nullptr);

  TabChangeWaiter tab_change_waiter(profile1());
  browser()->tab_strip_model()->CloseWebContentsAt(
      browser()->tab_strip_model()->active_index(), TabCloseTypes::CLOSE_NONE);
  tab_change_waiter.WaitForTabDataChanged({kFooDotComTitle1});
  TestGetSuggestedTopics({kTopic2}, nullptr);
}

IN_PROC_BROWSER_TEST_F(TabSearchPageHandlerBrowserTest,
                       GetSuggestedTopics_Navigated) {
  auto* mock_engine = SetMockTabOrganizationEngine();
  EXPECT_CALL(*mock_engine, GetSuggestedTopics(_, _))
      .WillOnce(
          base::test::RunOnceCallback<1>(std::vector<std::string>{kTopic}))
      .WillOnce(
          base::test::RunOnceCallback<1>(std::vector<std::string>{kTopic2}));

  AppendTabWithTitle(
      browser(), https_server()->GetURL("foo.com", "/simple.html"),
      kFooDotComTitle1, ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);
  TestGetSuggestedTopics({kTopic}, nullptr);

  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), https_server()->GetURL("dog.com", "/simple.html")));
  TestGetSuggestedTopics({kTopic2}, nullptr);
}

IN_PROC_BROWSER_TEST_F(TabSearchPageHandlerBrowserTest, GetFocusTabs) {
  // Create another browser with the default profile.
  ui_test_utils::BrowserCreatedObserver browser_created_observer;
  chrome::NewEmptyWindow(profile1(), false);
  Browser* browser2 = browser_created_observer.Wait();

  // Test Engine's GetFocusTabs is called with expected tabs info and topic.
  auto* mock_engine = SetMockTabOrganizationEngine();

  // Add tabs in windows with the default profile.
  AppendTabWithTitle(browser(), GURL(kFooDotComUrl1), kFooDotComTitle1);
  AppendTabWithTitle(browser(), GURL(kFooDotComUrl2), kFooDotComTitle2);
  AppendTabWithTitle(browser2, GURL(kBarDotComUrl1), kBarDotComTitle1);
  AppendTabWithTitle(browser2, GURL(kBarDotComUrl2), kBarDotComTitle2);

  // size is 3 because a blank tab is added by default when creating windows.
  ASSERT_EQ(browser()->tab_strip_model()->count(), 3);
  ASSERT_EQ(browser2->GetTabStripModel()->count(), 3);

  const int tab_id1 =
      browser()->tab_strip_model()->GetTabAtIndex(1)->GetHandle().raw_value();
  const int tab_id2 =
      browser()->tab_strip_model()->GetTabAtIndex(2)->GetHandle().raw_value();
  const int tab_id3 =
      browser2->GetTabStripModel()->GetTabAtIndex(1)->GetHandle().raw_value();
  const int tab_id4 =
      browser2->GetTabStripModel()->GetTabAtIndex(2)->GetHandle().raw_value();

  // Tabs order: browser() tabs first, then browser2 tabs.
  std::vector<ai_chat::Tab> expected_tabs_order1 = {
      {base::NumberToString(tab_id1), kFooDotComTitle1,
       url::Origin::Create(GURL(kFooDotComUrl1))},
      {base::NumberToString(tab_id2), kFooDotComTitle2,
       url::Origin::Create(GURL(kFooDotComUrl2))},
      {base::NumberToString(tab_id3), kBarDotComTitle1,
       url::Origin::Create(GURL(kBarDotComUrl1))},
      {base::NumberToString(tab_id4), kBarDotComTitle2,
       url::Origin::Create(GURL(kBarDotComUrl2))},
  };

  // Tabs order: browser2 tabs first, then browser() tabs.
  std::vector<ai_chat::Tab> expected_tabs_order2 = {
      {base::NumberToString(tab_id3), kBarDotComTitle1,
       url::Origin::Create(GURL(kBarDotComUrl1))},
      {base::NumberToString(tab_id4), kBarDotComTitle2,
       url::Origin::Create(GURL(kBarDotComUrl2))},
      {base::NumberToString(tab_id1), kFooDotComTitle1,
       url::Origin::Create(GURL(kFooDotComUrl1))},
      {base::NumberToString(tab_id2), kFooDotComTitle2,
       url::Origin::Create(GURL(kFooDotComUrl2))},
  };

  std::vector<std::string> mock_ret_tabs = {base::NumberToString(tab_id1),
                                            "100", "invalid",
                                            base::NumberToString(tab_id4)};

  // Accept either order since browser iteration order maybe non-deterministic.
  using testing::AnyOf;
  EXPECT_CALL(*mock_engine,
              GetFocusTabs(AnyOf(expected_tabs_order1, expected_tabs_order2),
                           "topic", _))
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
  ASSERT_EQ(GlobalBrowserCollection::GetInstance()->GetSize(), 3u)
      << "A new window should be created.";
  Browser* focus_tabs_browser =
      GetLastActiveBrowserWindowInterfaceWithAnyProfile()
          ->GetBrowserForMigrationOnly();
  EXPECT_EQ(focus_tabs_browser->GetTabStripModel()->count(), 2)
      << "The new window should have 2 tabs.";
  EXPECT_EQ(focus_tabs_browser->user_title(), "topic");

  // Check the tabs are moved to the new window as expected.
  EXPECT_EQ(focus_tabs_browser->GetTabStripModel()
                ->GetTabAtIndex(0)
                ->GetHandle()
                .raw_value(),
            tab_id1);
  EXPECT_EQ(focus_tabs_browser->GetTabStripModel()
                ->GetTabAtIndex(1)
                ->GetHandle()
                .raw_value(),
            tab_id4);

  // Test undo.
  base::RunLoop run_loop2;
  handler()->UndoFocusTabs(base::BindLambdaForTesting([&]() {
    // Focused tabs window should already be closed.
    ASSERT_EQ(GlobalBrowserCollection::GetInstance()->GetSize(), 2u);

    EXPECT_EQ(browser()->tab_strip_model()->count(), 3)
        << "The tabs should be moved back to the previous active window.";
    EXPECT_EQ(
        browser()->tab_strip_model()->GetTabAtIndex(1)->GetHandle().raw_value(),
        tab_id1);
    EXPECT_EQ(browser2->GetTabStripModel()->count(), 3)
        << "The tabs should be moved back to the previous active window.";
    EXPECT_EQ(
        browser2->GetTabStripModel()->GetTabAtIndex(2)->GetHandle().raw_value(),
        tab_id4);
    run_loop2.Quit();
  }));

  run_loop2.Run();
}

// End-to-end coverage of `SearchTabsByContent`: open-tab GURLs are resolved
// to URLIDs through `HistoryService::QueryUrlIds`, the URL-id filter is
// forwarded to `Search()`, and the scored rows are mapped back to tab
// handles.
class TabSearchPageHandlerSearchTabsByContentBrowserTest
    : public TabSearchPageHandlerBrowserTest {
 public:
  TabSearchPageHandlerSearchTabsByContentBrowserTest() {
    scoped_features_.InitAndEnableFeature(
        history_embeddings::kHistoryEmbeddings);
  }

 private:
  base::test::ScopedFeatureList scoped_features_;
};

IN_PROC_BROWSER_TEST_F(TabSearchPageHandlerSearchTabsByContentBrowserTest,
                       SearchTabsByContent) {
  // `IsHistoryEmbeddingsEnabledForProfile` also requires the Brave-owned pref.
  profile1()->GetPrefs()->SetBoolean(
      local_ai::prefs::kBraveHistoryEmbeddingsEnabled, true);

  AppendTabWithTitle(browser(),
                     https_server()->GetURL("foo.com", "/empty.html"),
                     kFooDotComTitle1);
  AppendTabWithTitle(browser(),
                     https_server()->GetURL("bar.com", "/empty.html"),
                     kBarDotComTitle1);

  const int foo_tab_id =
      browser()->tab_strip_model()->GetTabAtIndex(1)->GetHandle().raw_value();
  const int bar_tab_id =
      browser()->tab_strip_model()->GetTabAtIndex(2)->GetHandle().raw_value();
  const GURL foo_url =
      browser()->tab_strip_model()->GetTabAtIndex(1)->GetContents()->GetURL();
  const GURL bar_url =
      browser()->tab_strip_model()->GetTabAtIndex(2)->GetContents()->GetURL();

  // Wait until both URLs are indexed in history so QueryUrlIds returns
  // non-zero IDs for them. `AppendTabWithTitle` waits for load stop but the
  // history record is added on a separate task.
  history::HistoryService* history_service =
      HistoryServiceFactory::GetForProfile(profile1(),
                                           ServiceAccessType::EXPLICIT_ACCESS);
  ASSERT_TRUE(history_service);
  // Add the navigated URLs to history synchronously so the URLIDs are ready
  // by the time `SearchTabsByContent` runs `QueryUrlIds`.
  history_service->AddPage(foo_url, base::Time::Now(), /*context_id=*/0,
                           /*nav_entry_id=*/0, /*referrer=*/GURL(),
                           history::RedirectList(), ui::PAGE_TRANSITION_LINK,
                           history::SOURCE_BROWSED,
                           history::VisitResponseCodeCategory::kNot404,
                           /*did_replace_entry=*/false);
  history_service->AddPage(bar_url, base::Time::Now(), /*context_id=*/0,
                           /*nav_entry_id=*/0, /*referrer=*/GURL(),
                           history::RedirectList(), ui::PAGE_TRANSITION_LINK,
                           history::SOURCE_BROWSED,
                           history::VisitResponseCodeCategory::kNot404,
                           /*did_replace_entry=*/false);

  base::CancelableTaskTracker tracker;
  const auto query_url_id = [&](const GURL& url) -> history::URLID {
    base::test::TestFuture<history::QueryURLResult> future;
    history_service->QueryURL(url, future.GetCallback(), &tracker);
    const auto result = future.Take();
    return result.success ? result.row.id() : 0;
  };
  const history::URLID foo_url_id = query_url_id(foo_url);
  const history::URLID bar_url_id = query_url_id(bar_url);
  ASSERT_NE(foo_url_id, 0);
  ASSERT_NE(bar_url_id, 0);

  ai_chat::FakeHistoryEmbeddingsSearch fake;
  fake.SetScoredRows({
      ai_chat::FakeHistoryEmbeddingsSearch::MakeRow(
          bar_url_id, bar_url, base::UTF8ToUTF16(std::string(kBarDotComTitle1)),
          base::Time::Now(), /*score=*/0.9f),
      ai_chat::FakeHistoryEmbeddingsSearch::MakeRow(
          foo_url_id, foo_url, base::UTF8ToUTF16(std::string(kFooDotComTitle1)),
          base::Time::Now(), /*score=*/0.7f),
  });
  handler()->SetEmbeddingsSearchForTesting(&fake);

  base::test::TestFuture<const std::vector<int32_t>&> future;
  handler()->SearchTabsByContent("query", future.GetCallback());
  const std::vector<int32_t> tab_ids = future.Take();

  // The page handler forwards to `open_tab_search` and returns the matched
  // open-tab ids; ranking order and the URL-id filter it builds are covered
  // by open_tab_search's own browser test.
  EXPECT_THAT(tab_ids, testing::UnorderedElementsAre(foo_tab_id, bar_tab_id));

  handler()->SetEmbeddingsSearchForTesting(nullptr);
}
