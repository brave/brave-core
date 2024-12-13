// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/direct_feed_fetcher.h"

#include "base/test/bind.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/net/system_network_context_manager.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"
#include "testing/gtest/include/gtest/gtest.h"

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
  DirectFeedFetcherBrowserTest() = default;

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    host_resolver()->AddRule("*", "127.0.0.1");

    https_server_.RegisterRequestHandler(base::BindLambdaForTesting(
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
        }));
    fetcher_ = std::make_unique<DirectFeedFetcher>(
        g_browser_process->system_network_context_manager()
            ->GetSharedURLLoaderFactory(),
        delegate_.AsWeakPtr());

    ASSERT_TRUE(https_server_.Start());
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

  net::EmbeddedTestServer https_server_{net::EmbeddedTestServer::TYPE_HTTPS};
  content::ContentMockCertVerifier mock_cert_verifier_;
  MockDelegate delegate_;
  std::unique_ptr<DirectFeedFetcher> fetcher_;
};

IN_PROC_BROWSER_TEST_F(DirectFeedFetcherBrowserTest, RedirectToFeed) {
  base::RunLoop run_loop;
  GURL feed2_url = https_server_.GetURL("/feed2");

  fetcher_->DownloadFeed(
      feed2_url, "test_publisher",
      base::BindLambdaForTesting([&](DirectFeedResponse response) {
        const auto& result = absl::get<DirectFeedResult>(response.result);
        ASSERT_EQ(1u, result.articles.size());
        EXPECT_EQ(feed2_url.spec(), response.url.spec());
        EXPECT_EQ("Hacker News", result.title);
        run_loop.Quit();
      }));

  run_loop.Run();
}

}  // namespace brave_news
