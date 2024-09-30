// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/peeking_card.h"

#include <tuple>
#include <utility>
#include <vector>

#include "base/containers/flat_map.h"
#include "brave/components/brave_news/browser/feed_sampling.h"
#include "brave/components/brave_news/common/brave_news.mojom.h"
#include "brave/components/brave_news/common/subscriptions_snapshot.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_news {

namespace {

ArticleInfo FakeInfo(std::string id = "",
                     std::vector<std::string> channels = {},
                     GURL url = GURL()) {
  static int next_id = 0;

  if (id.empty()) {
    id = base::NumberToString(++next_id);
  }

  auto item = mojom::FeedItemMetadata::New();
  item->publisher_id = id;
  item->channels = channels;
  item->url = url;

  return std::make_tuple(std::move(item), ArticleMetadata());
}

}  // namespace

TEST(BraveNewsPeekingCard, NoArticles) {
  EXPECT_EQ(std::nullopt, PickPeekingCard(SubscriptionsSnapshot(), {}, {}));
}

TEST(BraveNewsPeekingCard, NoSubscribedArticles) {
  ArticleInfos articles;
  articles.push_back(FakeInfo());
  articles.push_back(FakeInfo());
  articles.push_back(FakeInfo());
  EXPECT_EQ(std::nullopt,
            PickPeekingCard(SubscriptionsSnapshot(), {}, articles));
}

TEST(BraveNewsPeekingCard, PublishersAreIncluded) {
  ArticleInfos articles;
  articles.push_back(FakeInfo());
  articles.push_back(FakeInfo());
  articles.push_back(FakeInfo());

  size_t enabled_index = 1;
  SubscriptionsSnapshot subscriptions(
      {std::get<0>(articles[enabled_index])->publisher_id}, {}, {}, {});

  EXPECT_EQ(enabled_index, PickPeekingCardWithMax(subscriptions, {}, articles,
                                                  /*max_candidates=*/1));
}

TEST(BraveNewsPeekingCard, ChannelsAreIncluded) {
  ArticleInfos articles;
  articles.push_back(FakeInfo());
  articles.push_back(FakeInfo("foo", {"one"}));
  articles.push_back(FakeInfo());

  base::flat_map<std::string, std::vector<std::string>> channels;
  channels["en_NZ"] = {"one"};
  SubscriptionsSnapshot subscriptions({}, {}, {}, channels);

  EXPECT_EQ(1, PickPeekingCardWithMax(subscriptions, {}, articles,
                                      /*max_candidates=*/1));
}

TEST(BraveNewsPeekingCard, DirectFeedsAreIncluded) {
  ArticleInfos articles;
  articles.push_back(FakeInfo());
  articles.push_back(FakeInfo("foo"));
  articles.push_back(FakeInfo());

  std::vector<DirectFeed> feeds;
  feeds.push_back({"foo", GURL(), ""});
  SubscriptionsSnapshot subscriptions({}, {}, feeds, {});

  EXPECT_EQ(1, PickPeekingCardWithMax(subscriptions, {}, articles,
                                      /*max_candidates=*/1));
}

TEST(BraveNewsPeekingCard, DisabledPublishersExcluded) {
  ArticleInfos articles;
  articles.push_back(FakeInfo("bar"));
  articles.push_back(FakeInfo("foo"));
  articles.push_back(FakeInfo("frob"));

  SubscriptionsSnapshot subscriptions({"foo"}, {"foo", "bar", "frob"}, {}, {});

  EXPECT_EQ(std::nullopt, PickPeekingCardWithMax(subscriptions, {}, articles,
                                                 /*max_candidates=*/1));
}

TEST(BraveNewsPeekingCard, DirectFeedsAreHigherThanPublishers) {
  ArticleInfos articles;
  articles.push_back(FakeInfo("combined"));
  articles.push_back(FakeInfo("direct"));
  articles.push_back(FakeInfo("other"));

  std::vector<DirectFeed> feeds;
  feeds.push_back({"direct", GURL(), ""});
  SubscriptionsSnapshot subscriptions({"other", "combined"}, {}, feeds, {});

  EXPECT_EQ(1, PickPeekingCardWithMax(subscriptions, {}, articles,
                                      /*max_candidates=*/1));
}

TEST(BraveNewsPeekingCard, PublishersAreHigherThanChannels) {
  ArticleInfos articles;
  articles.push_back(FakeInfo("", {"one", "two"}));
  articles.push_back(FakeInfo("combined"));
  articles.push_back(FakeInfo("", {"two"}));

  base::flat_map<std::string, std::vector<std::string>> channels;
  channels["en_NZ"] = {"one", "two"};

  SubscriptionsSnapshot subscriptions({"other", "combined"}, {}, {}, channels);

  EXPECT_EQ(1, PickPeekingCardWithMax(subscriptions, {}, articles,
                                      /*max_candidates=*/1));
}

TEST(BraveNewsPeekingCard, TopNewsBoost) {
  ArticleInfos articles;
  articles.push_back(FakeInfo("one", {}, GURL("https://one.com/1")));
  articles.push_back(FakeInfo("one", {}, GURL("https://one.com/2")));
  articles.push_back(FakeInfo("one", {}, GURL("https://one.com/3")));

  SubscriptionsSnapshot subscriptions({"one"}, {}, {}, {});

  EXPECT_EQ(1, PickPeekingCardWithMax(subscriptions, {"https://one.com/2"},
                                      articles, /*max_candidates=*/1));
}

}  // namespace brave_news
