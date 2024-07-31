// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/brave_news/brave_news_tab_helper.h"

#include <optional>
#include <string>
#include <vector>

#include "base/command_line.h"
#include "base/containers/flat_map.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/scoped_observation.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_news/common/pref_names.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "content/public/test/test_navigation_observer.h"
#include "content/public/test/test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "ui/base/page_transition_types.h"
#include "ui/base/window_open_disposition.h"

namespace {
class WaitForFeedsChanged : public BraveNewsTabHelper::PageFeedsObserver {
 public:
  WaitForFeedsChanged(BraveNewsTabHelper* tab_helper,
                      std::optional<size_t> expected_feed_count)
      : expected_feed_count_(expected_feed_count), tab_helper_(tab_helper) {
    news_observer_.Observe(tab_helper_);
  }

  ~WaitForFeedsChanged() override = default;

  std::vector<GURL> WaitForFeeds() {
    if (!last_feeds_ || last_feeds_->size() != expected_feed_count_) {
      loop_.Run();
    }

    return last_feeds_.value();
  }

 private:
  void OnAvailableFeedsChanged(const std::vector<GURL>& feeds) override {
    // There can be multiple OnAvailableFeedsChanged events, as we navigate
    // (first to clear, then again to populate). This class is waiting for
    // feeds, so expect to receive some.
    if (expected_feed_count_.has_value() &&
        feeds.size() != expected_feed_count_) {
      return;
    }

    last_feeds_ = feeds;
    loop_.Quit();
  }

  std::optional<size_t> expected_feed_count_ = 0;
  base::RunLoop loop_;
  raw_ptr<BraveNewsTabHelper> tab_helper_;
  std::optional<std::vector<GURL>> last_feeds_ = std::nullopt;

  base::ScopedObservation<BraveNewsTabHelper,
                          BraveNewsTabHelper::PageFeedsObserver>
      news_observer_{this};
};

class WaitForFeedTitle {
 public:
  explicit WaitForFeedTitle(BraveNewsTabHelper* tab_helper)
      : tab_helper_(tab_helper) {}

  ~WaitForFeedTitle() = default;

  bool WaitForTitle(std::string title) {
    bool found_title = false;
    do {
      WaitForFeedsChanged waiter(tab_helper_.get(), std::nullopt);
      auto urls = waiter.WaitForFeeds();
      found_title = base::ranges::any_of(urls, [&title, this](const auto& url) {
        return title == tab_helper_->GetTitleForFeedUrl(url);
      });
    } while (!found_title);
    return true;
  }

 private:
  raw_ptr<BraveNewsTabHelper> tab_helper_;
  std::string title_;
};

}  // namespace

class BraveNewsTabHelperTest : public InProcessBrowserTest {
 public:
  BraveNewsTabHelperTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {}

  void OptIn() {
    auto* prefs = browser()->profile()->GetPrefs();
    prefs->SetBoolean(brave_news::prefs::kNewTabPageShowToday, true);
    prefs->SetBoolean(brave_news::prefs::kBraveNewsOptedIn, true);
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
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    https_server_.ServeFilesFromDirectory(test_data_dir);
    https_server_.AddDefaultHandlers(GetChromeTestDataDir());
    host_resolver()->AddRule("*", "127.0.0.1");
    cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
  }

  content::WebContents* contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  net::EmbeddedTestServer* https_server() { return &https_server_; }

 private:
  net::EmbeddedTestServer https_server_;
  content::ContentMockCertVerifier cert_verifier_;
};

IN_PROC_BROWSER_TEST_F(BraveNewsTabHelperTest, TabHelperIsCreated) {
  EXPECT_NE(nullptr, BraveNewsTabHelper::FromWebContents(contents()));
}

IN_PROC_BROWSER_TEST_F(BraveNewsTabHelperTest,
                       TabHelperNotifiesObserversWhenFoundFeeds) {
  auto* tab_helper = BraveNewsTabHelper::FromWebContents(contents());
  WaitForFeedsChanged waiter(tab_helper, 2);

  tab_helper->OnReceivedRssUrls(
      contents()->GetLastCommittedURL(),
      {GURL("https://example.com/1"), GURL("https://example.com/2")});

  auto result = waiter.WaitForFeeds();
  EXPECT_EQ(2u, result.size());
}

IN_PROC_BROWSER_TEST_F(BraveNewsTabHelperTest, FeedsAreDeduplicated) {
  auto* tab_helper = BraveNewsTabHelper::FromWebContents(contents());
  WaitForFeedsChanged waiter(tab_helper, 1);

  GURL url("https://example.com/1");
  tab_helper->OnReceivedRssUrls(contents()->GetLastCommittedURL(), {url, url});

  auto result = waiter.WaitForFeeds();
  EXPECT_EQ(1u, result.size());
  EXPECT_EQ(url, result[0]);
}

IN_PROC_BROWSER_TEST_F(BraveNewsTabHelperTest, NonExistingFeedsAreRemoved) {
  OptIn();

  ASSERT_TRUE(https_server()->Start());
  GURL rss_page_url = https_server()->GetURL("/page_with_bad_rss.html");

  auto* tab_helper = BraveNewsTabHelper::FromWebContents(contents());

  GURL feed_url;
  {
    WaitForFeedsChanged waiter(tab_helper, 1);

    ui_test_utils::NavigateToURLWithDisposition(
        browser(), rss_page_url, WindowOpenDisposition::CURRENT_TAB,
        ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);
    auto result = waiter.WaitForFeeds();

    ASSERT_EQ(1u, result.size());
    feed_url = result[0];
    EXPECT_EQ(https_server()->GetURL("/rss_feed_which_does_not_exist.xml"),
              feed_url);
  }

  // At first, as we haven't tried to fetch the RSS feed, we don't know it's
  // invalid. When we receive the change notification, we should have removed
  // the invalid feed.
  {
    WaitForFeedsChanged waiter(tab_helper, 0);
    EXPECT_EQ(feed_url.spec(), tab_helper->GetTitleForFeedUrl(feed_url));

    auto result = waiter.WaitForFeeds();
    EXPECT_EQ(0u, result.size());
  }
}

IN_PROC_BROWSER_TEST_F(BraveNewsTabHelperTest, FeedsAreFoundWhenTheyExist) {
  OptIn();

  ASSERT_TRUE(https_server()->Start());
  GURL rss_page_url = https_server()->GetURL("/page_with_rss.html");

  auto* tab_helper = BraveNewsTabHelper::FromWebContents(contents());

  GURL feed_url;
  {
    WaitForFeedsChanged waiter(tab_helper, 1);

    ui_test_utils::NavigateToURLWithDisposition(
        browser(), rss_page_url, WindowOpenDisposition::CURRENT_TAB,
        ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);
    auto result = waiter.WaitForFeeds();

    ASSERT_EQ(1u, result.size());
    feed_url = result[0];
    EXPECT_EQ(https_server()->GetURL("/page_with_rss.xml"), feed_url);
  }

  // At first, we should not have loaded the title (and fallback to the feed
  // url). Requesting the title should trigger fetching and parsing the feed to
  // get the title.
  {
    WaitForFeedTitle waiter(tab_helper);
    EXPECT_EQ(feed_url.spec(), tab_helper->GetTitleForFeedUrl(feed_url));
    EXPECT_TRUE(waiter.WaitForTitle("Channel Title"));

    // Once the feed has been parsed, we should be notified that we have
    // changes.
    EXPECT_EQ("Channel Title", tab_helper->GetTitleForFeedUrl(feed_url));
  }
}

IN_PROC_BROWSER_TEST_F(BraveNewsTabHelperTest, FeedsAreNotFoundWhenNotOptedIn) {
  ASSERT_TRUE(https_server()->Start());
  GURL rss_page_url = https_server()->GetURL("/page_with_rss.html");

  auto* tab_helper = BraveNewsTabHelper::FromWebContents(contents());

  ui_test_utils::NavigateToURLWithDisposition(
      browser(), rss_page_url, WindowOpenDisposition::CURRENT_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);

  // We run until idle here rather than using the Waiter because we want the
  // load to complete before notifying, and we don't notify empty results.
  base::RunLoop().RunUntilIdle();

  ASSERT_EQ(0u, tab_helper->GetAvailableFeedUrls().size());
}
