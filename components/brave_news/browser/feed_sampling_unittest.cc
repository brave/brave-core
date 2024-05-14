// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/feed_sampling.h"

#include <string>
#include <tuple>
#include <vector>

#include "base/containers/contains.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_news/browser/feed_fetcher.h"
#include "brave/components/brave_news/browser/publishers_controller.h"
#include "brave/components/brave_news/browser/signal_calculator.h"
#include "brave/components/brave_news/common/brave_news.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/googletest/src/googletest/include/gtest/gtest.h"

namespace brave_news {

namespace {

std::tuple<std::string, FeedItems, Publishers, Signals>
GetDataForArticleInfos() {
  std::string locale("en_NZ");
  FeedItems items;
  Publishers publishers;

  auto p1 = mojom::Publisher::New();
  p1->publisher_id = "one";
  p1->user_enabled_status = mojom::UserEnabled::ENABLED;
  publishers["one"] = std::move(p1);

  auto p2 = mojom::Publisher::New();
  p2->publisher_id = "two";
  publishers["two"] = std::move(p2);

  auto pDisabled = mojom::Publisher::New();
  pDisabled->publisher_id = "disabled";
  pDisabled->user_enabled_status = mojom::UserEnabled::DISABLED;
  publishers["disabled"] = std::move(pDisabled);

  Signals signals;
  signals["one"] = mojom::Signal::New(false, 5, 0.8, 5);
  signals["two"] = mojom::Signal::New(false, 0, 0, 100);
  signals["channel"] = mojom::Signal::New(false, 10, 0.3, 100);
  signals["disabled"] = mojom::Signal::New(true, 10, 0.1, 100);

  return std::make_tuple(locale, std::move(items), std::move(publishers),
                         std::move(signals));
}

mojom::FeedItemPtr MakeArticleItem(const std::string& publisher_id) {
  static size_t generated_articles = 0;

  auto result = mojom::FeedItemMetadata::New();
  result->publisher_id = publisher_id;
  result->url = GURL(
      base::StrCat({std::string("https://"), publisher_id, std::string(".com/"),
                    base::NumberToString(generated_articles)}));
  // result.
  return mojom::FeedItem::NewArticle(
      mojom::Article::New(std::move(result), false));
}

}  // namespace

TEST(BraveNewsFeedSampling, CanPickRandomItem) {
  constexpr int iterations = 100;
  std::vector<int> ints = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

  for (auto i = 0; i < iterations; ++i) {
    auto result = PickRandom(ints);
    EXPECT_TRUE(base::Contains(ints, result));
  }

  std::vector<std::string> strings = {"foo", "bar", "hello"};
  for (auto i = 0; i < iterations; ++i) {
    const auto& result = PickRandom(strings);
    EXPECT_TRUE(base::Contains(strings, result));
  }
}

TEST(BraveNewsFeedSampling, CanSampleContentGroupEmpty) {
  std::vector<ContentGroup> groups;
  auto [name, is_channel] = SampleContentGroup(groups);
  EXPECT_EQ("", name);
  EXPECT_FALSE(is_channel);
}

TEST(BraveNewsFeedSampling, CanSampleContentGroup) {
  constexpr int iterations = 100;
  std::vector<ContentGroup> groups = {{"publisher_1", false},
                                      {"publisher_2", false},
                                      {"channel_1", true},
                                      {"channel_2", true},
                                      {"publisher_3", false}};

  for (auto i = 0; i < iterations; ++i) {
    auto sample = SampleContentGroup(groups);
    EXPECT_TRUE(base::Contains(groups, sample));
  }
}

TEST(BraveNewsFeedSampling, GetNormalIsClampedBetweenZeroAndOne) {
  constexpr int iterations = 1000;
  for (auto i = 0; i < iterations; ++i) {
    auto normal = GetNormal();
    EXPECT_GE(normal, 0);
    EXPECT_LE(normal, 1);
  }
}

TEST(BraveNewsFeedSampling, PickFirstIndexPicksFirstUnlessArticlesAreEmpty) {
  ArticleInfos infos;
  EXPECT_EQ(std::nullopt, PickFirstIndex(infos));

  infos.push_back({mojom::FeedItemMetadata::New(), ArticleWeight()});

  EXPECT_EQ(0u, PickFirstIndex(infos).value());

  infos.push_back({mojom::FeedItemMetadata::New(), ArticleWeight()});
  infos.push_back({mojom::FeedItemMetadata::New(), ArticleWeight()});
  infos.push_back({mojom::FeedItemMetadata::New(), ArticleWeight()});

  EXPECT_EQ(0u, PickFirstIndex(infos).value());
}

TEST(BraveNewsFeedSampling, PickRouletteDoesntBreakOnEmptyList) {
  ArticleInfos infos;

  EXPECT_EQ(std::nullopt, PickRoulette(infos));
}

TEST(BraveNewsFeedSampling, PickRouletteWithWeighting) {
  ArticleInfos infos;
  infos.push_back({mojom::FeedItemMetadata::New(), ArticleWeight()});
  infos.push_back({mojom::FeedItemMetadata::New(), ArticleWeight()});
  infos.push_back({mojom::FeedItemMetadata::New(), ArticleWeight()});

  // No positively weighted items, so we shouldn't pick anything.
  EXPECT_EQ(std::nullopt,
            PickRouletteWithWeighting(
                infos, base::BindRepeating(
                           [](const mojom::FeedItemMetadataPtr& item,
                              const ArticleWeight& weight) { return 0.0; })));
  auto& first = std::get<0>(infos.at(0));
  auto& second = std::get<0>(infos.at(1));
  auto& third = std::get<0>(infos.at(2));
  auto make_picker_for =
      [](const mojom::FeedItemMetadataPtr& target) -> GetWeighting {
    return base::BindRepeating(
        [](mojom::FeedItemMetadata* target,
           const mojom::FeedItemMetadataPtr& item,
           const ArticleWeight& weight) {
          return target == item.get() ? 100.0 : 0.0;
        },
        target.get());
  };

  EXPECT_EQ(0, PickRouletteWithWeighting(infos, make_picker_for(first)));
  EXPECT_EQ(1, PickRouletteWithWeighting(infos, make_picker_for(second)));
  EXPECT_EQ(2, PickRouletteWithWeighting(infos, make_picker_for(third)));
}

TEST(BraveNewsFeedSampling, GetArticleInfosSkipsNull) {
  auto [locale, items, publishers, signals] = GetDataForArticleInfos();
  items.push_back(nullptr);

  EXPECT_EQ(0u, GetArticleInfos(locale, items, publishers, signals).size());
}

TEST(BraveNewsFeedSampling, GetArticleInfosSkipsNonArticles) {
  auto [locale, items, publishers, signals] = GetDataForArticleInfos();

  items.push_back(MakeArticleItem("one"));
  items.push_back(MakeArticleItem("two"));
  items.push_back(mojom::FeedItem::NewDeal(mojom::Deal::New()));

  EXPECT_EQ(2u, GetArticleInfos(locale, items, publishers, signals).size());
}

TEST(BraveNewsFeedSampling, GetArticleInfosDuplicatesExcluded) {
  auto [locale, items, publishers, signals] = GetDataForArticleInfos();

  auto item = MakeArticleItem("one");
  items.push_back(item->Clone());
  items.push_back(std::move(item));

  EXPECT_EQ(1u, GetArticleInfos(locale, items, publishers, signals).size());
}
TEST(BraveNewsFeedSampling, GetArticleInfosUnknownPublishersSkipped) {
  auto [locale, items, publishers, signals] = GetDataForArticleInfos();

  items.push_back(MakeArticleItem("one"));
  items.push_back(MakeArticleItem("not-a-real-publisher"));

  EXPECT_EQ(1u, GetArticleInfos(locale, items, publishers, signals).size());
}

TEST(BraveNewsFeedSampling, GetArticleInfosDisabledArticlesExcluded) {
  auto [locale, items, publishers, signals] = GetDataForArticleInfos();

  items.push_back(MakeArticleItem("disabled"));
  items.push_back(MakeArticleItem("one"));

  EXPECT_EQ(1u, GetArticleInfos(locale, items, publishers, signals).size());
}

TEST(BraveNewsFeedSampling, GetArticleInfosUsesCorrectSignals) {
  auto [locale, items, publishers, signals] = GetDataForArticleInfos();

  items.push_back(MakeArticleItem("disabled"));
  items.push_back(MakeArticleItem("one"));
  items.push_back(MakeArticleItem("two"));
  items.push_back(MakeArticleItem("not-a-real-publisher"));

  auto infos = GetArticleInfos(locale, items, publishers, signals);
  EXPECT_EQ(2u, infos.size());

  auto& [article0, weight0] = infos.at(0);
  EXPECT_EQ("one", article0->publisher_id);
  EXPECT_TRUE(weight0.visited);
  EXPECT_TRUE(weight0.subscribed);

  auto& [article1, weight1] = infos.at(1);
  EXPECT_EQ("two", article1->publisher_id);
  EXPECT_FALSE(weight1.visited);
  EXPECT_FALSE(weight1.subscribed);
}

}  // namespace brave_news
