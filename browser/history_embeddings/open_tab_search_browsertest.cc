// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/history_embeddings/open_tab_search.h"

#include <cstdint>
#include <string>
#include <vector>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/cancelable_task_tracker.h"
#include "base/test/test_future.h"
#include "base/time/time.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/history_embeddings/test/fake_history_embeddings_search.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/history/core/browser/history_service.h"
#include "components/history/core/browser/history_types.h"
#include "components/keyed_service/core/service_access_type.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/page_transition_types.h"
#include "url/gurl.h"

namespace history_embeddings {

class OpenTabSearchBrowserTest : public InProcessBrowserTest {
 public:
  OpenTabSearchBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {}

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    base::FilePath test_data_dir =
        base::PathService::CheckedGet(brave::DIR_TEST_DATA);
    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    https_server_.ServeFilesFromDirectory(test_data_dir);
    host_resolver()->AddRule("*", "127.0.0.1");
    ASSERT_TRUE(https_server_.Start());
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

  Profile* profile() { return browser()->profile(); }

  GURL GetURL(const std::string& host, const std::string& path) {
    return https_server_.GetURL(host, path);
  }

  void AppendTab(Browser* target, const GURL& url, const std::string& title) {
    ui_test_utils::NavigateToURLWithDisposition(
        target, url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
        ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);
    content::WebContents* contents =
        target->tab_strip_model()->GetActiveWebContents();
    contents->UpdateTitleForEntry(
        contents->GetController().GetLastCommittedEntry(),
        base::UTF8ToUTF16(title));
  }

  int TabIdAt(Browser* target, int index) {
    return target->tab_strip_model()
        ->GetTabAtIndex(index)
        ->GetHandle()
        .raw_value();
  }

  history::HistoryService* history_service() {
    return HistoryServiceFactory::GetForProfile(
        profile(), ServiceAccessType::EXPLICIT_ACCESS);
  }

  void AddToHistory(const GURL& url) {
    history_service()->AddPage(
        url, base::Time::Now(), /*context_id=*/0, /*nav_entry_id=*/0,
        /*referrer=*/GURL(), history::RedirectList(), ui::PAGE_TRANSITION_LINK,
        history::SOURCE_BROWSED, history::VisitResponseCodeCategory::kNot404,
        /*did_replace_entry=*/false);
  }

  history::URLID QueryUrlId(const GURL& url) {
    base::test::TestFuture<history::QueryURLResult> future;
    history_service()->QueryURL(url, future.GetCallback(), &tracker_);
    const history::QueryURLResult result = future.Take();
    return result.success ? result.row.id() : 0;
  }

 protected:
  base::CancelableTaskTracker tracker_;

 private:
  net::EmbeddedTestServer https_server_;
  content::ContentMockCertVerifier mock_cert_verifier_;
};

// Results follow the scored-row order (best first), and open tabs whose URL
// isn't among the scored rows are dropped.
IN_PROC_BROWSER_TEST_F(OpenTabSearchBrowserTest, RanksAndDropsUnmatchedTabs) {
  const GURL foo_url = GetURL("foo.com", "/empty.html");
  const GURL bar_url = GetURL("bar.com", "/empty.html");
  const GURL baz_url = GetURL("baz.com", "/empty.html");
  AppendTab(browser(), foo_url, "Foo");
  AppendTab(browser(), bar_url, "Bar");
  AppendTab(browser(), baz_url, "Baz");

  const int foo_tab_id = TabIdAt(browser(), 1);
  const int bar_tab_id = TabIdAt(browser(), 2);

  AddToHistory(foo_url);
  AddToHistory(bar_url);
  AddToHistory(baz_url);
  const history::URLID foo_url_id = QueryUrlId(foo_url);
  const history::URLID bar_url_id = QueryUrlId(bar_url);
  const history::URLID baz_url_id = QueryUrlId(baz_url);
  ASSERT_NE(foo_url_id, 0);
  ASSERT_NE(bar_url_id, 0);
  ASSERT_NE(baz_url_id, 0);

  // Score bar above foo; leave baz unscored.
  ai_chat::FakeHistoryEmbeddingsSearch fake;
  fake.SetScoredRows({
      ai_chat::FakeHistoryEmbeddingsSearch::MakeRow(
          bar_url_id, bar_url, u"Bar", base::Time::Now(), /*score=*/0.9f),
      ai_chat::FakeHistoryEmbeddingsSearch::MakeRow(
          foo_url_id, foo_url, u"Foo", base::Time::Now(), /*score=*/0.7f),
  });

  base::test::TestFuture<std::vector<OpenTabInfo>> future;
  SearchOpenTabsByContent(profile(), history_service(), &fake, "query",
                          future.GetCallback(), &tracker_);
  const std::vector<OpenTabInfo> ranked = future.Take();

  // Every eligible open tab is offered to `Search()` for scoring...
  EXPECT_TRUE(fake.last_skip_answering());
  EXPECT_THAT(
      fake.last_url_id_filter(),
      testing::UnorderedElementsAre(foo_url_id, bar_url_id, baz_url_id));
  // ...but only the scored ones come back, best first; baz is dropped.
  std::vector<int32_t> tab_ids;
  for (const auto& tab : ranked) {
    tab_ids.push_back(tab.tab_id);
  }
  EXPECT_THAT(tab_ids, testing::ElementsAre(bar_tab_id, foo_tab_id));
}

// Tabs from a different profile (here, incognito) never contribute, even when
// their URL is present and scored in the searched profile's history.
IN_PROC_BROWSER_TEST_F(OpenTabSearchBrowserTest, ExcludesOtherProfileTabs) {
  const GURL foo_url = GetURL("foo.com", "/empty.html");
  const GURL bar_url = GetURL("bar.com", "/empty.html");
  AppendTab(browser(), foo_url, "Foo");
  Browser* incognito = CreateIncognitoBrowser();
  AppendTab(incognito, bar_url, "Bar (incognito)");

  const int foo_tab_id = TabIdAt(browser(), 1);

  // Both URLs are in the regular profile's history and both are scored, so the
  // only reason bar can be excluded is the tracking filter dropping the
  // incognito tab before its URL is ever resolved.
  AddToHistory(foo_url);
  AddToHistory(bar_url);
  const history::URLID foo_url_id = QueryUrlId(foo_url);
  const history::URLID bar_url_id = QueryUrlId(bar_url);
  ASSERT_NE(foo_url_id, 0);
  ASSERT_NE(bar_url_id, 0);

  ai_chat::FakeHistoryEmbeddingsSearch fake;
  fake.SetScoredRows({
      ai_chat::FakeHistoryEmbeddingsSearch::MakeRow(
          foo_url_id, foo_url, u"Foo", base::Time::Now(), /*score=*/0.9f),
      ai_chat::FakeHistoryEmbeddingsSearch::MakeRow(
          bar_url_id, bar_url, u"Bar", base::Time::Now(), /*score=*/0.8f),
  });

  base::test::TestFuture<std::vector<OpenTabInfo>> future;
  SearchOpenTabsByContent(profile(), history_service(), &fake, "query",
                          future.GetCallback(), &tracker_);
  const std::vector<OpenTabInfo> ranked = future.Take();

  // Only the regular-profile tab reaches the URL-id filter and the results.
  EXPECT_THAT(fake.last_url_id_filter(), testing::ElementsAre(foo_url_id));
  ASSERT_EQ(ranked.size(), 1u);
  EXPECT_EQ(ranked[0].tab_id, foo_tab_id);
  EXPECT_EQ(ranked[0].url, foo_url);
}

}  // namespace history_embeddings
