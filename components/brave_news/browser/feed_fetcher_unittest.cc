// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/feed_fetcher.h"

#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "brave/components/brave_news/common/brave_news.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace brave_news {

// Befriended by FeedFetcher so it can exercise the otherwise-private
// FeedSourceResult aggregation logic without spinning up the network stack.
class FeedFetcherTest : public testing::Test {
 protected:
  using FeedSourceResult = FeedFetcher::FeedSourceResult;

  static mojom::FeedItemPtr MakeArticle(const std::string& url) {
    auto metadata = mojom::FeedItemMetadata::New();
    metadata->url = GURL(url);
    auto article = mojom::Article::New();
    article->data = std::move(metadata);
    return mojom::FeedItem::NewArticle(std::move(article));
  }

  static FeedSourceResult MakeResult(bool success,
                                     bool connection_error,
                                     std::vector<std::string> article_urls) {
    FeedSourceResult result;
    result.success = success;
    result.connection_error = connection_error;
    for (const auto& url : article_urls) {
      result.items.push_back(MakeArticle(url));
    }
    return result;
  }

  // Routes access to the private static through the befriended fixture, since
  // gtest runs each TEST_F body in a derived class that doesn't inherit
  // friendship.
  static std::tuple<FeedItems, ETags, bool> Combine(
      std::vector<FeedSourceResult> results) {
    return FeedFetcher::CombineFeedSourceResults(std::move(results));
  }

  static bool CombineHasConnectionError(std::vector<FeedSourceResult> results) {
    return std::get<2>(Combine(std::move(results)));
  }
};

// One source's inputs for a connection-error test case. Kept copyable (unlike
// the move-only FeedSourceResult) so cases can live in a static table.
struct SourceSpec {
  bool success;
  bool connection_error;
  std::vector<std::string> article_urls;
};

struct ConnectionErrorTestCase {
  std::string test_name;
  std::vector<SourceSpec> sources;
  bool expected_connection_error;
};

class FeedFetcherConnectionErrorTest
    : public FeedFetcherTest,
      public testing::WithParamInterface<ConnectionErrorTestCase> {};

TEST_P(FeedFetcherConnectionErrorTest, ReportsConnectionError) {
  const ConnectionErrorTestCase& test_case = GetParam();
  SCOPED_TRACE(test_case.test_name);
  std::vector<FeedSourceResult> results;
  for (const auto& source : test_case.sources) {
    results.push_back(MakeResult(source.success, source.connection_error,
                                 source.article_urls));
  }
  EXPECT_EQ(test_case.expected_connection_error,
            CombineHasConnectionError(std::move(results)));
}

INSTANTIATE_TEST_SUITE_P(
    ,
    FeedFetcherConnectionErrorTest,
    testing::ValuesIn(std::vector<ConnectionErrorTestCase>{
        // A source that failed to reach Brave's feed server (and nothing else
        // succeeded) is a genuine connection error.
        {"ServerConnectionFailure",
         {{/*success=*/false, /*connection_error=*/true, /*article_urls=*/{}}},
         /*expected_connection_error=*/true},
        // The core of the bug: a direct (custom RSS) feed that fails to load
        // must not be reported as a whole-feed connection error, even when it
        // is the only source. A failing direct feed never sets
        // |connection_error|.
        {"DirectFeedFailureAlone",
         {{/*success=*/false, /*connection_error=*/false, /*article_urls=*/{}}},
         /*expected_connection_error=*/false},
        // If any source loaded (proving we're online) we don't report a
        // connection error, even if another source hit a network failure.
        {"ConnectionFailureIgnoredWhenAnotherSourceSucceeds",
         {{/*success=*/false, /*connection_error=*/true, /*article_urls=*/{}},
          {/*success=*/true, /*connection_error=*/false, /*article_urls=*/{}}},
         /*expected_connection_error=*/false},
        // A successful fetch is never a connection error.
        {"SuccessfulFetch",
         {{/*success=*/true, /*connection_error=*/false,
           /*article_urls=*/{"https://example.com/a"}}},
         /*expected_connection_error=*/false},
        // With no sources at all there's nothing to blame on the connection.
        {"NoSources", {}, /*expected_connection_error=*/false},
    }),
    [](const testing::TestParamInfo<ConnectionErrorTestCase>& info) {
      return info.param.test_name;
    });

// Combining still drops articles with invalid URLs while keeping valid ones,
// so a feed with one bad entry and one good entry yields the good article.
TEST_F(FeedFetcherTest, CombineKeepsValidArticlesAndDropsInvalid) {
  std::vector<FeedSourceResult> results;
  results.push_back(MakeResult(
      /*success=*/true, /*connection_error=*/false,
      // The first URL is malformed (no host), standing in for the invalid
      // entry in the issue's repro feed; the second is valid.
      /*article_urls=*/{"https://", "https://example.com/"}));

  auto [items, etags, connection_error] = Combine(std::move(results));
  ASSERT_EQ(1u, items.size());
  ASSERT_TRUE(items[0]->is_article());
  EXPECT_EQ("https://example.com/", items[0]->get_article()->data->url.spec());
  EXPECT_FALSE(connection_error);
}

}  // namespace brave_news
