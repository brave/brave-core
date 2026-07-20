// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/tools/tab_semantic_search_tool.h"

#include <string>
#include <vector>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/json/json_reader.h"
#include "base/path_service.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/cancelable_task_tracker.h"
#include "base/test/test_future.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
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

namespace ai_chat {

class TabSemanticSearchToolBrowserTest : public InProcessBrowserTest {
 public:
  TabSemanticSearchToolBrowserTest()
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

  void AppendTab(const GURL& url, const std::string& title) {
    ui_test_utils::NavigateToURLWithDisposition(
        browser(), url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
        ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);
    content::WebContents* contents =
        browser()->tab_strip_model()->GetActiveWebContents();
    contents->UpdateTitleForEntry(
        contents->GetController().GetLastCommittedEntry(),
        base::UTF8ToUTF16(title));
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

// A non-empty ranked result yields both the model-facing text and a
// `kTabSourcesArtifactType` artifact whose payload carries every matched tab.
IN_PROC_BROWSER_TEST_F(TabSemanticSearchToolBrowserTest,
                       EmitsTabSourcesArtifact) {
  const GURL foo_url = GetURL("foo.com", "/empty.html");
  const GURL bar_url = GetURL("bar.com", "/empty.html");
  AppendTab(foo_url, "Foo Tab");
  AppendTab(bar_url, "Bar Tab");

  const int foo_tab_id =
      browser()->tab_strip_model()->GetTabAtIndex(1)->GetHandle().raw_value();
  const int bar_tab_id =
      browser()->tab_strip_model()->GetTabAtIndex(2)->GetHandle().raw_value();

  AddToHistory(foo_url);
  AddToHistory(bar_url);
  const history::URLID foo_url_id = QueryUrlId(foo_url);
  const history::URLID bar_url_id = QueryUrlId(bar_url);
  ASSERT_NE(foo_url_id, 0);
  ASSERT_NE(bar_url_id, 0);

  FakeHistoryEmbeddingsSearch fake;
  fake.SetScoredRows({
      FakeHistoryEmbeddingsSearch::MakeRow(bar_url_id, bar_url, u"Bar Tab",
                                           base::Time::Now(), /*score=*/0.9f),
      FakeHistoryEmbeddingsSearch::MakeRow(foo_url_id, foo_url, u"Foo Tab",
                                           base::Time::Now(), /*score=*/0.7f),
  });

  TabSemanticSearchTool tool(profile());
  tool.UserPermissionGranted("1");
  tool.SetEmbeddingsSearchForTesting(&fake);

  base::test::TestFuture<std::vector<mojom::ContentBlockPtr>,
                         std::vector<mojom::ToolArtifactPtr>>
      future;
  tool.UseTool(R"({"query":"tabs"})", future.GetCallback());
  const auto& artifacts = future.Get<std::vector<mojom::ToolArtifactPtr>>();

  ASSERT_EQ(artifacts.size(), 1u);
  EXPECT_EQ(artifacts[0]->type, mojom::kTabSourcesArtifactType);

  auto parsed = base::JSONReader::ReadDict(artifacts[0]->content_json,
                                           base::JSON_PARSE_RFC);
  ASSERT_TRUE(parsed.has_value());
  const auto* sources = parsed->FindList("sources");
  ASSERT_TRUE(sources);
  ASSERT_EQ(sources->size(), 2u);
  // Best-first order matches the scored rows.
  EXPECT_EQ(*(*sources)[0].GetDict().FindInt("tab_id"), bar_tab_id);
  EXPECT_EQ(*(*sources)[1].GetDict().FindInt("tab_id"), foo_tab_id);
}

}  // namespace ai_chat
