// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/brave_news/brave_news_tab_helper.h"

#include <algorithm>
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
#include "base/test/test_future.h"
#include "brave/browser/brave_news/brave_news_controller_factory.h"
#include "brave/components/brave_news/browser/brave_news_controller.h"
#include "brave/components/brave_news/browser/brave_news_pref_manager.h"
#include "brave/components/brave_news/common/brave_news.mojom.h"
#include "brave/components/brave_news/common/pref_names.h"
#include "brave/components/brave_news/common/subscriptions_snapshot.h"
#include "brave/components/brave_news/common/types.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "content/public/test/test_navigation_observer.h"
#include "content/public/test/test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "services/network/public/cpp/ip_address_space_overrides_test_utils.h"
#include "services/network/public/mojom/ip_address_space.mojom.h"
#include "ui/base/page_transition_types.h"
#include "ui/base/window_open_disposition.h"
#include "url/gurl.h"
#include "url/origin.h"

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
      found_title = std::ranges::any_of(urls, [&title, this](const auto& url) {
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
  BraveNewsTabHelperTest() {
    // Ports need to be assigned before we can add IP address space overrides
    // for the servers on the command line.
    CHECK(https_server_.InitializeAndListen());
    CHECK(local_network_server_.InitializeAndListen());
  }

  void OptIn() {
    auto* prefs = browser()->GetProfile()->GetPrefs();
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
    InProcessBrowserTest::SetUpCommandLine(command_line);
    command_line->AppendSwitch(switches::kAllowRunningInsecureContent);
    cert_verifier_.SetUpCommandLine(command_line);
    // Everything resolves to 127.0.0.1 in browser tests, which means every test
    // server looks like it's on the loopback network. Pretend |https_server_|
    // is a normal internet host, so that |local_network_server_| can be used to
    // represent a host which resolves to a local address.
    network::AddIpAddressSpaceOverridesToCommandLine(
        {network::GenerateIpAddressSpaceOverride(
             https_server_, network::mojom::IPAddressSpace::kPublic),
         network::GenerateIpAddressSpaceOverride(
             local_network_server_, network::mojom::IPAddressSpace::kLoopback)},
        *command_line);
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    https_server_.ServeFilesFromDirectory(test_data_dir);
    https_server_.AddDefaultHandlers(GetChromeTestDataDir());
    local_network_server_.ServeFilesFromDirectory(test_data_dir);
    local_network_server_.AddDefaultHandlers(GetChromeTestDataDir());
    host_resolver()->AddRule("*", "127.0.0.1");
    cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    https_server_.StartAcceptingConnections();
    local_network_server_.StartAcceptingConnections();
  }

  content::WebContents* contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  brave_news::BraveNewsController* controller() {
    return brave_news::BraveNewsControllerFactory::GetForBrowserContext(
        browser()->GetProfile());
  }

  net::EmbeddedTestServer* https_server() { return &https_server_; }

  // A server which, from the network service's point of view, is on the
  // loopback network, even though its URLs use a regular hostname. This is the
  // situation an attacker creates by pointing a DNS record at 127.0.0.1.
  net::EmbeddedTestServer* local_network_server() {
    return &local_network_server_;
  }

 private:
  net::EmbeddedTestServer https_server_{net::EmbeddedTestServer::TYPE_HTTPS};
  net::EmbeddedTestServer local_network_server_{
      net::EmbeddedTestServer::TYPE_HTTPS};
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

  GURL rss_page_url =
      https_server()->GetURL("example.com", "/page_with_bad_rss.html");

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
    EXPECT_EQ(https_server()->GetURL("example.com",
                                     "/rss_feed_which_does_not_exist.xml"),
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

  GURL rss_page_url =
      https_server()->GetURL("example.com", "/page_with_rss.html");

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
    EXPECT_EQ(https_server()->GetURL("example.com", "/page_with_rss.xml"),
              feed_url);
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

// Feed URLs which are obviously on the local network are never offered, so a
// page can't get us to make a request to a service running on the user's
// machine or local network.
// See https://github.com/brave/brave-browser/issues/56884.
IN_PROC_BROWSER_TEST_F(BraveNewsTabHelperTest,
                       LocalNetworkFeedUrlsAreNotAvailable) {
  auto* tab_helper = BraveNewsTabHelper::FromWebContents(contents());
  WaitForFeedsChanged waiter(tab_helper, 1);

  const GURL allowed_feed("https://example.com/feed.xml");
  tab_helper->OnReceivedRssUrls(
      contents()->GetLastCommittedURL(),
      {
          // Loopback, in all its guises.
          GURL("http://localhost/feed.xml"),
          GURL("http://foo.localhost/feed.xml"),
          GURL("http://127.0.0.1/feed.xml"),
          GURL("http://127.1.2.3/feed.xml"),
          GURL("http://0177.0000.0000.0001/feed.xml"),
          GURL("http://0x7F000001/feed.xml"),
          GURL("http://2130706433/feed.xml"),
          GURL("http://[::1]/feed.xml"),
          GURL("http://[::ffff:127.0.0.1]/feed.xml"),
          GURL("http://[::ffff:7f00:1]/feed.xml"),
          // 0.0.0.0 and friends, which also end up on the local machine.
          GURL("http://0.0.0.0/feed.xml"),
          GURL("http://0/feed.xml"),
          GURL("http://[::]/feed.xml"),
          GURL("http://[::ffff:0.0.0.0]/feed.xml"),
          // Local network addresses.
          GURL("http://10.0.0.1/feed.xml"),
          GURL("http://172.16.0.1/feed.xml"),
          GURL("http://192.168.0.1/feed.xml"),
          GURL("http://169.254.0.1/feed.xml"),
          GURL("http://100.64.0.1/feed.xml"),
          GURL("http://[fc00::1]/feed.xml"),
          GURL("http://[fe80::1]/feed.xml"),
          GURL("http://router.local/feed.xml"),
          // ...and one which should be left alone.
          allowed_feed,
      });

  auto result = waiter.WaitForFeeds();
  ASSERT_EQ(1u, result.size());
  EXPECT_EQ(allowed_feed, result[0]);
}

// A feed URL can point at the local network without that being apparent from
// the URL, by using a hostname whose DNS record points at a local address. In
// that case the request is blocked once the address has been resolved, and the
// feed is dropped.
// See https://github.com/brave/brave-browser/issues/56884.
IN_PROC_BROWSER_TEST_F(BraveNewsTabHelperTest,
                       FeedUrlsWhichResolveToLocalAddressesAreRemoved) {
  OptIn();

  auto* tab_helper = BraveNewsTabHelper::FromWebContents(contents());

  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), https_server()->GetURL("example.com", "/simple.html")));

  // This looks like a regular internet host, but resolves into the loopback
  // address space.
  const GURL feed_url =
      local_network_server()->GetURL("feed.example.com", "/page_with_rss.xml");

  {
    WaitForFeedsChanged waiter(tab_helper, 1);
    tab_helper->OnReceivedRssUrls(contents()->GetLastCommittedURL(),
                                  {feed_url});
    auto result = waiter.WaitForFeeds();
    ASSERT_EQ(1u, result.size());
    EXPECT_EQ(feed_url, result[0]);
  }

  // Requesting the title fetches the feed. The fetch should be blocked, which
  // makes the feed look non-existent, so it gets removed.
  {
    WaitForFeedsChanged waiter(tab_helper, 0);
    EXPECT_EQ(feed_url.spec(), tab_helper->GetTitleForFeedUrl(feed_url));

    auto result = waiter.WaitForFeeds();
    EXPECT_EQ(0u, result.size());
  }
}

// Feeds which resolve to a local address can't be subscribed to from a page,
// but the user can still add them themselves from the Brave News settings.
// See https://github.com/brave/brave-browser/issues/56884.
IN_PROC_BROWSER_TEST_F(BraveNewsTabHelperTest,
                       CantSubscribeToFeedsWhichResolveToLocalAddresses) {
  OptIn();

  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), https_server()->GetURL("example.com", "/simple.html")));
  const auto initiator_origin =
      contents()->GetPrimaryMainFrame()->GetLastCommittedOrigin();

  const GURL feed_url =
      local_network_server()->GetURL("feed.example.com", "/page_with_rss.xml");

  auto subscribe = [&](const std::optional<url::Origin>& initiator) {
    base::test::TestFuture<bool, bool,
                           std::optional<brave_news::MojomPublishers>>
        future;
    controller()->SubscribeToNewDirectFeed(feed_url, initiator,
                                           future.GetCallback());
    return future.Get<0>();
  };

  // On behalf of the page: blocked, and nothing gets added to the user's
  // subscriptions.
  EXPECT_FALSE(subscribe(initiator_origin));
  EXPECT_TRUE(controller()->prefs().GetSubscriptions().direct_feeds().empty());

  // Added by the user: allowed, and the feed shows up in the subscriptions.
  EXPECT_TRUE(subscribe(std::nullopt));
  const auto subscriptions = controller()->prefs().GetSubscriptions();
  ASSERT_EQ(1u, subscriptions.direct_feeds().size());
  EXPECT_EQ(feed_url, subscriptions.direct_feeds()[0].url);
}
