// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include <vector>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/scoped_observation.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/brave_news/brave_news_tab_helper.h"
#include "brave/components/brave_today/common/brave_news.mojom-forward.h"
#include "brave/components/brave_today/common/features.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_navigator_params.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "content/public/test/test_navigation_observer.h"
#include "content/public/test/test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "ui/base/page_transition_types.h"
#include "ui/base/window_open_disposition.h"

namespace {
class WaitForFeedsChanged : public BraveNewsTabHelper::PageFeedsObserver {
 public:
  explicit WaitForFeedsChanged(BraveNewsTabHelper* tab_helper)
      : tab_helper_(tab_helper) {
    news_observer_.Observe(tab_helper_);
  }

  ~WaitForFeedsChanged() override = default;

  std::vector<BraveNewsTabHelper::FeedDetails> WaitForChange() {
    if (!last_feeds_)
      loop_.Run();
    return last_feeds_.value();
  }

 private:
  void OnAvailableFeedsChanged(
      const std::vector<BraveNewsTabHelper::FeedDetails>& feeds) override {
    // There can be multiple OnAvailableFeedsChanged events, as we navigate
    // (first to clear, then again to populate). This class is waiting for
    // feeds, so expect to receive some.
    if (feeds.size() == 0)
      return;

    last_feeds_ = feeds;
    loop_.Quit();
  }

  base::RunLoop loop_;
  raw_ptr<BraveNewsTabHelper> tab_helper_;
  absl::optional<std::vector<BraveNewsTabHelper::FeedDetails>> last_feeds_ =
      absl::nullopt;

  base::ScopedObservation<BraveNewsTabHelper,
                          BraveNewsTabHelper::PageFeedsObserver>
      news_observer_{this};
};
}  // namespace

class BraveNewsTabHelperTest : public InProcessBrowserTest {
 public:
  BraveNewsTabHelperTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {
    features_.InitWithFeatures(
        {brave_today::features::kBraveNewsSubscribeButtonFeature}, {});
  }

  void SetUp() override {
    brave::RegisterPathProvider();
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    https_server_.ServeFilesFromDirectory(test_data_dir);
    https_server_.AddDefaultHandlers(GetChromeTestDataDir());

    InProcessBrowserTest::SetUp();
  }

  void SetUpInProcessBrowserTestFixture() override {
    InProcessBrowserTest::SetUpInProcessBrowserTestFixture();
    cert_verifier_.SetUpInProcessBrowserTestFixture();
  }

  void TearDownInProcessBrowserTestFixture() override {
    InProcessBrowserTest::TearDownInProcessBrowserTestFixture();
    cert_verifier_.TearDownInProcessBrowserTestFixture();
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    command_line->AppendSwitch(switches::kAllowRunningInsecureContent);
    cert_verifier_.SetUpCommandLine(command_line);
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");
    cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
  }

  content::WebContents* contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  net::EmbeddedTestServer* https_server() { return &https_server_; }

 private:
  base::test::ScopedFeatureList features_;
  net::EmbeddedTestServer https_server_;
  content::ContentMockCertVerifier cert_verifier_;
};

IN_PROC_BROWSER_TEST_F(BraveNewsTabHelperTest, TabHelperIsCreated) {
  EXPECT_NE(nullptr, BraveNewsTabHelper::FromWebContents(contents()));
}

IN_PROC_BROWSER_TEST_F(BraveNewsTabHelperTest,
                       TabHelperNotifiesObserversWhenFoundFeeds) {
  auto* tab_helper = BraveNewsTabHelper::FromWebContents(contents());
  WaitForFeedsChanged waiter(tab_helper);

  std::vector<brave_news::mojom::FeedSearchResultItemPtr> feeds;
  auto feed1 = brave_news::mojom::FeedSearchResultItem::New();
  feed1->feed_url = GURL("https://example.com/1");

  auto feed2 = brave_news::mojom::FeedSearchResultItem::New();
  feed2->feed_url = GURL("https://example.com/2");
  feeds.emplace_back(std::move(feed1));
  feeds.emplace_back(std::move(feed2));
  tab_helper->OnFoundFeeds(contents()->GetLastCommittedURL(), std::move(feeds));

  auto result = waiter.WaitForChange();
  EXPECT_EQ(2u, result.size());
}

IN_PROC_BROWSER_TEST_F(BraveNewsTabHelperTest, FeedsAreDeduplicated) {
  auto* tab_helper = BraveNewsTabHelper::FromWebContents(contents());
  WaitForFeedsChanged waiter(tab_helper);

  std::vector<brave_news::mojom::FeedSearchResultItemPtr> feeds;
  auto feed1 = brave_news::mojom::FeedSearchResultItem::New();
  feed1->feed_url = GURL("https://example.com/1");
  auto feed1_copy = feed1.Clone();
  feeds.emplace_back(std::move(feed1));
  feeds.emplace_back(std::move(feed1_copy));
  tab_helper->OnFoundFeeds(contents()->GetLastCommittedURL(), std::move(feeds));

  auto result = waiter.WaitForChange();
  EXPECT_EQ(1u, result.size());
}

IN_PROC_BROWSER_TEST_F(BraveNewsTabHelperTest, FeedsAreFoundWhenTheyExists) {
  ASSERT_TRUE(https_server()->Start());
  GURL rss_page_url = https_server()->GetURL("/page_with_rss.html");

  auto* tab_helper = BraveNewsTabHelper::FromWebContents(contents());
  WaitForFeedsChanged waiter(tab_helper);

  ui_test_utils::NavigateToURLWithDisposition(
      browser(), rss_page_url, WindowOpenDisposition::CURRENT_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);

  auto result = waiter.WaitForChange();

  ASSERT_EQ(1u, result.size());
  EXPECT_EQ("Channel Title", result[0].title);
  EXPECT_EQ(https_server()->GetURL("/page_with_rss.xml"), result[0].feed_url);
}
