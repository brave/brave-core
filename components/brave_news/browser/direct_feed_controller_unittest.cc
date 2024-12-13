// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/direct_feed_controller.h"

#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/memory/scoped_refptr.h"
#include "brave/components/brave_news/browser/brave_news_pref_manager.h"
#include "brave/components/brave_news/browser/test/wait_for_callback.h"
#include "content/public/test/browser_task_environment.h"
#include "services/data_decoder/public/cpp/test_support/in_process_data_decoder.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
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

std::string GetHtmlPageWithFeed() {
  return R"(<!doctype html>
    <html lang=en>
      <head>
        <meta charset=utf-8>
        <title>Page</title>
        <link rel="alternate" type="application/rss+xml" title="RSS" href="/feed.xml" />
      </head>
      <body>
        <p>I'm the content</p>
      </body>
    </html>)";
}

std::string GetHtmlPageWithNoFeed() {
  return R"(<!doctype html>
    <html lang=en>
      <head>
        <meta charset=utf-8>
        <title>Page</title>
      </head>
      <body>
        <p>I'm the content</p>
      </body>
    </html>)";
}

std::string GetPlainText() {
  return "Hello World";
}

constexpr char kPageUrl[] = "https://example.com";
constexpr char kFeedURL[] = "https://example.com/feed";

}  // namespace
class BraveNewsDirectFeedControllerTest : public testing::Test {
 public:
  BraveNewsDirectFeedControllerTest()
      : direct_feed_controller_(test_url_loader_factory_.GetSafeWeakWrapper(),
                                direct_feed_fetcher_delegate_.AsWeakPtr()) {}

 protected:
  class MockDirectFeedFetcherDelegate : public DirectFeedFetcher::Delegate {
   public:
    ~MockDirectFeedFetcherDelegate() override = default;

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
    base::WeakPtrFactory<MockDirectFeedFetcherDelegate> weak_ptr_factory_{this};
  };

  std::tuple<bool, std::string> VerifyFeedUrl(GURL feed_url) {
    return WaitForCallback(base::BindOnce(
        &DirectFeedController::VerifyFeedUrl,
        base::Unretained(&direct_feed_controller_), std::move(feed_url)));
  }

  std::vector<mojom::FeedSearchResultItemPtr> FindFeeds(
      const GURL& possible_feed_or_site_url) {
    auto [feeds] = WaitForCallback(base::BindOnce(
        &DirectFeedController::FindFeeds,
        base::Unretained(&direct_feed_controller_), possible_feed_or_site_url));
    return std::move(feeds);
  }

  content::BrowserTaskEnvironment task_environment_;
  data_decoder::test::InProcessDataDecoder data_decoder_;
  network::TestURLLoaderFactory test_url_loader_factory_;
  MockDirectFeedFetcherDelegate direct_feed_fetcher_delegate_;
  DirectFeedController direct_feed_controller_;
};

TEST_F(BraveNewsDirectFeedControllerTest, ValidFeedURLIsVerified) {
  // Fetch a RSS feed
  test_url_loader_factory_.AddResponse(kFeedURL, GetBasicFeed());

  auto [valid, title] = VerifyFeedUrl(GURL(kFeedURL));
  EXPECT_TRUE(valid);
  EXPECT_EQ("Hacker News", title);
}

TEST_F(BraveNewsDirectFeedControllerTest, InvalidFeedURLIsNotVerified) {
  // Fetch a non RSS resource
  test_url_loader_factory_.AddResponse(kFeedURL, GetPlainText());

  auto [valid, title] = VerifyFeedUrl(GURL(kFeedURL));
  EXPECT_FALSE(valid);
  EXPECT_EQ("", title);
}

TEST_F(BraveNewsDirectFeedControllerTest, ErrorResponseIsNotVerified) {
  // Fetch a non RSS resource
  test_url_loader_factory_.AddResponse(kFeedURL, GetBasicFeed(),
                                       net::HTTP_NOT_FOUND);

  auto [valid, title] = VerifyFeedUrl(GURL(kFeedURL));
  EXPECT_FALSE(valid);
  EXPECT_EQ("", title);
}

TEST_F(BraveNewsDirectFeedControllerTest, CanFindFeedFromFeedURL) {
  // Find an RSS feed
  test_url_loader_factory_.AddResponse(kFeedURL, GetBasicFeed());
  auto result = FindFeeds(GURL(kFeedURL));

  ASSERT_EQ(1u, result.size());
  auto feed = std::move(result.at(0));
  EXPECT_EQ(kFeedURL, feed->feed_url.spec());
  EXPECT_EQ("Hacker News", feed->feed_title);
}

TEST_F(BraveNewsDirectFeedControllerTest, CanUpgradeToHTTPS) {
  // Find an RSS feed
  test_url_loader_factory_.AddResponse(kFeedURL, GetBasicFeed());
  auto result = FindFeeds(GURL("http://example.com/feed"));

  ASSERT_EQ(1u, result.size());
  auto feed = std::move(result.at(0));
  EXPECT_EQ("http://example.com/feed", feed->feed_url.spec());
  EXPECT_EQ("Hacker News", feed->feed_title);
}

TEST_F(BraveNewsDirectFeedControllerTest, CanFindFeedFromPageWithFeedURL) {
  // Fetch a page with an RSS feed
  test_url_loader_factory_.AddResponse(kPageUrl, GetHtmlPageWithFeed());

  // Set the response for the RSS feed
  test_url_loader_factory_.AddResponse(kFeedURL, GetBasicFeed());

  auto result = FindFeeds(GURL(kFeedURL));

  ASSERT_EQ(1u, result.size());
  auto feed = std::move(result.at(0));
  EXPECT_EQ(kFeedURL, feed->feed_url.spec());
  EXPECT_EQ("Hacker News", feed->feed_title);
}

TEST_F(BraveNewsDirectFeedControllerTest, DontFindFeedOnPageWithNoFeedURL) {
  // Fetch a HTML page with no RSS feed
  test_url_loader_factory_.AddResponse(kPageUrl, GetHtmlPageWithNoFeed());

  auto result = FindFeeds(GURL(kPageUrl));
  EXPECT_TRUE(result.empty());
}

TEST_F(BraveNewsDirectFeedControllerTest, DontFindFeedOnNonPageNonFeedURL) {
  // Fetch some random file
  test_url_loader_factory_.AddResponse(kPageUrl, GetPlainText());

  auto result = FindFeeds(GURL(kPageUrl));
  EXPECT_TRUE(result.empty());
}

}  // namespace brave_news
