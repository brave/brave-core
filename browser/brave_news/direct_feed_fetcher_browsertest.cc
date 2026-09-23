// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/direct_feed_fetcher.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>

#include "base/command_line.h"
#include "base/logging.h"
#include "base/run_loop.h"
#include "base/test/bind.h"
#include "base/test/test_future.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/net/system_network_context_manager.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"
#include "services/network/public/cpp/ip_address_space_overrides_test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace brave_news {

namespace {
std::string GetBasicFeed() {
  return R"(<rss version="2.0">
    <channel>
      <title>Hacker News</title>
      <link>https://news.ycombinator.com/</link>
      <description>Links for the intellectually curious, ranked by readers.</description>
      <item>
        <title>Enough with the dead butterflies (2017)</title>
        <link>https://www.emilydamstra.com/please-enough-dead-butterflies/</link>
        <pubDate>Sun, 3 Mar 2024 22:40:13 +0000</pubDate>
        <comments>https://news.ycombinator.com/item?id=39585207</comments>
        <description><![CDATA[<a href="https://news.ycombinator.com/item?id=39585207">Comments</a>]]></description>
      </item>
    </channel>
  </rss>)";
}

}  // namespace

class DirectFeedFetcherBrowserTest : public InProcessBrowserTest {
 public:
  DirectFeedFetcherBrowserTest() {
    // The port needs to be assigned before we can add IP address space
    // overrides for the server on the command line.
    CHECK(public_server_.InitializeAndListen());
  }

  void SetUpInProcessBrowserTestFixture() override {
    InProcessBrowserTest::SetUpInProcessBrowserTestFixture();
    mock_cert_verifier_.SetUpInProcessBrowserTestFixture();
  }

  void TearDownInProcessBrowserTestFixture() override {
    mock_cert_verifier_.TearDownInProcessBrowserTestFixture();
    InProcessBrowserTest::TearDownInProcessBrowserTestFixture();
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpCommandLine(command_line);
    mock_cert_verifier_.SetUpCommandLine(command_line);
    // Everything resolves to 127.0.0.1 in browser tests, which means every test
    // server looks like it's on the loopback network. Pretend |public_server_|
    // is a normal internet host, so that |https_server_| can be used to
    // represent a host which resolves to a local address.
    network::AddPublicIpAddressSpaceOverrideToCommandLine(public_server_,
                                                          *command_line);
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    host_resolver()->AddRule("*", "127.0.0.1");

    auto feed_handler = base::BindLambdaForTesting(
        [](const net::test_server::HttpRequest& request)
            -> std::unique_ptr<net::test_server::HttpResponse> {
          LOG(ERROR) << "request to https " << request.GetURL().path();
          if (request.GetURL().path() == "/feed") {
            auto response =
                std::make_unique<net::test_server::BasicHttpResponse>();
            response->set_code(net::HTTP_OK);
            response->set_content(GetBasicFeed());
            response->set_content_type("application/rss+xml");
            return response;
          } else if (request.GetURL().path() == "/feed2") {
            auto response =
                std::make_unique<net::test_server::BasicHttpResponse>();
            response->set_code(net::HTTP_MOVED_PERMANENTLY);
            response->AddCustomHeader("Location", "/feed");
            return response;
          }
          return nullptr;
        });
    https_server_.RegisterRequestHandler(feed_handler);
    public_server_.RegisterRequestHandler(feed_handler);

    fetcher_ = std::make_unique<DirectFeedFetcher>(
        g_browser_process->system_network_context_manager()
            ->GetSharedURLLoaderFactory(),
        delegate_.AsWeakPtr());

    ASSERT_TRUE(https_server_.Start());
    public_server_.StartAcceptingConnections();
  }

  // Fetches the feed at |url| on behalf of |initiator_origin| (or on behalf of
  // the user, if it's unset), returning whether the fetch succeeded.
  bool DownloadFeed(const GURL& url,
                    std::optional<url::Origin> initiator_origin) {
    base::test::TestFuture<DirectFeedResponse> future;
    fetcher_->DownloadFeed(url, std::move(initiator_origin), "test_publisher",
                           future.GetCallback());
    return std::holds_alternative<DirectFeedResult>(future.Get().result);
  }

 protected:
  class MockDelegate : public DirectFeedFetcher::Delegate {
   public:
    ~MockDelegate() override = default;

    DirectFeedFetcher::Delegate::HTTPSUpgradeInfo GetURLHTTPSUpgradeInfo(
        const GURL& url) override {
      HTTPSUpgradeInfo info;
      info.should_upgrade = true;
      info.should_force = false;
      return info;
    }

    base::WeakPtr<DirectFeedFetcher::Delegate> AsWeakPtr() override {
      return weak_ptr_factory_.GetWeakPtr();
    }

   private:
    base::WeakPtrFactory<MockDelegate> weak_ptr_factory_{this};
  };

  // Treated as being on the loopback network (browser tests resolve everything
  // to 127.0.0.1).
  net::EmbeddedTestServer https_server_{net::EmbeddedTestServer::TYPE_HTTPS};
  // Overridden to look like it's in the public IP address space.
  net::EmbeddedTestServer public_server_{net::EmbeddedTestServer::TYPE_HTTPS};
  content::ContentMockCertVerifier mock_cert_verifier_;
  MockDelegate delegate_;
  std::unique_ptr<DirectFeedFetcher> fetcher_;
};

IN_PROC_BROWSER_TEST_F(DirectFeedFetcherBrowserTest, RedirectToFeed) {
  base::RunLoop run_loop;
  GURL feed2_url = https_server_.GetURL("/feed2");

  fetcher_->DownloadFeed(
      feed2_url, std::nullopt, "test_publisher",
      base::BindLambdaForTesting([&](DirectFeedResponse response) {
        const auto& result = std::get<DirectFeedResult>(response.result);
        ASSERT_EQ(1u, result.articles.size());
        EXPECT_EQ(feed2_url.spec(), response.url.spec());
        EXPECT_EQ("Hacker News", result.title);
        run_loop.Quit();
      }));

  run_loop.Run();
}

// Fetches made on behalf of a web page (i.e. with an initiator origin) must not
// be able to reach the local network, even when the URL doesn't look local -
// e.g. a public hostname with a DNS record pointing at 127.0.0.1.
// See https://github.com/brave/brave-browser/issues/56884.
IN_PROC_BROWSER_TEST_F(DirectFeedFetcherBrowserTest,
                       PageInitiatedFetchesCantReachTheLocalNetwork) {
  // This looks like a regular internet host, but resolves to 127.0.0.1.
  const GURL local_feed_url = https_server_.GetURL("feed.example.com", "/feed");

  const auto initiator_origin =
      url::Origin::Create(public_server_.GetURL("example.com", "/"));

  // A feed the user asked for directly (no initiator origin) is allowed to be
  // on the local network.
  EXPECT_TRUE(DownloadFeed(local_feed_url, std::nullopt));

  // The same feed must not be fetchable on behalf of a page.
  EXPECT_FALSE(DownloadFeed(local_feed_url, initiator_origin));

  // Non-local feeds are still fetchable on behalf of a page.
  EXPECT_TRUE(DownloadFeed(public_server_.GetURL("feed.example.com", "/feed"),
                           initiator_origin));
}

}  // namespace brave_news
