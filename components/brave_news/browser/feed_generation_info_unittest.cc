// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/feed_generation_info.h"

#include <cstddef>
#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "base/containers/contains.h"
#include "base/containers/flat_map.h"
#include "base/functional/bind.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_news/browser/channels_controller.h"
#include "brave/components/brave_news/browser/feed_fetcher.h"
#include "brave/components/brave_news/browser/feed_sampling.h"
#include "brave/components/brave_news/browser/publishers_controller.h"
#include "brave/components/brave_news/browser/topics_fetcher.h"
#include "brave/components/brave_news/common/brave_news.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_news {

namespace {
constexpr char kFooChannel[] = "Foo";

constexpr char kPublisher1[] = "one";
constexpr char kPublisher2[] = "two";
constexpr char kPublisherDisabled[] = "disabled";

std::tuple<std::string, FeedItems, Publishers, Signals, TopicsResult>
MockFetchedData() {
  std::string locale("en_NZ");
  FeedItems items;
  Publishers publishers;

  auto p1 = mojom::Publisher::New();
  p1->publisher_id = kPublisher1;
  p1->user_enabled_status = mojom::UserEnabled::ENABLED;
  p1->locales.push_back(mojom::LocaleInfo::New(
      "en_NZ", 0, std::vector<std::string>{kTopNewsChannel, kFooChannel}));
  p1->locales.push_back(mojom::LocaleInfo::New(
      "en_AU", 0, std::vector<std::string>{kFooChannel}));
  publishers[kPublisher1] = std::move(p1);

  auto p2 = mojom::Publisher::New();
  p2->publisher_id = kPublisher2;
  p2->locales.push_back(mojom::LocaleInfo::New(
      "en_NZ", 0, std::vector<std::string>{kFooChannel}));
  publishers[kPublisher2] = std::move(p2);

  auto p_disabled = mojom::Publisher::New();
  p_disabled->publisher_id = kPublisherDisabled;
  p_disabled->user_enabled_status = mojom::UserEnabled::DISABLED;
  p_disabled->locales.push_back(mojom::LocaleInfo::New(
      "en_NZ", 0, std::vector<std::string>{kTopNewsChannel}));
  publishers[kPublisherDisabled] = std::move(p_disabled);

  Signals signals;
  signals[kPublisher1] = mojom::Signal::New(false, 5, 0.8, 5);
  signals[kPublisher2] = mojom::Signal::New(false, 0, 0, 100);
  signals[kFooChannel] = mojom::Signal::New(false, 10, 0.3, 100);
  signals[kTopNewsChannel] = mojom::Signal::New(false, 10, 0.3, 100);
  signals[kPublisherDisabled] = mojom::Signal::New(true, 10, 0.1, 100);

  TopicsResult topics;

  return std::make_tuple(locale, std::move(items), std::move(publishers),
                         std::move(signals), std::move(topics));
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

class BraveNewsFeedGenerationInfoTest : public testing::Test {
 public:
  BraveNewsFeedGenerationInfoTest() = default;
  BraveNewsFeedGenerationInfoTest(const BraveNewsFeedGenerationInfoTest&) =
      delete;
  BraveNewsFeedGenerationInfoTest& operator=(
      const BraveNewsFeedGenerationInfoTest&) = delete;
  ~BraveNewsFeedGenerationInfoTest() override = default;

  bool HasCreatedContentGroups(FeedGenerationInfo& info) {
    return info.content_groups_.has_value();
  }

  bool HasCreatedArticleInfos(FeedGenerationInfo& info) {
    return info.article_infos_.has_value();
  }

  base::flat_map<std::string, size_t>& GetAvailableCounts(
      FeedGenerationInfo& info) {
    return info.available_counts_;
  }
};

TEST_F(BraveNewsFeedGenerationInfoTest, CanCreateFeedGenerationInfo) {
  auto [locale, feed_items, publishers, signals, topics] = MockFetchedData();
  feed_items.push_back(MakeArticleItem(kPublisher1));
  feed_items.push_back(MakeArticleItem(kPublisher2));
  feed_items.push_back(MakeArticleItem(kPublisherDisabled));
  FeedGenerationInfo info(SubscriptionsSnapshot(), "en_NZ", feed_items,
                          publishers, {kTopNewsChannel, kFooChannel}, signals,
                          {}, topics);

  // ContentGroups and ArticleInfos should be lazily created.
  EXPECT_FALSE(HasCreatedArticleInfos(info));
  EXPECT_FALSE(HasCreatedContentGroups(info));

  // Articles from disabled publisher should be removed
  EXPECT_EQ(2u, info.GetArticleInfos().size());
  EXPECT_TRUE(HasCreatedArticleInfos(info));

  auto& content_groups = info.GetEligibleContentGroups();
  EXPECT_TRUE(HasCreatedContentGroups(info));

  // Content groups should include subscribed channels and publishers. Note:
  // There are only 3 items because PublisherTwo is not explicitly subscribed.
  EXPECT_EQ(3u, content_groups.size());

  auto has_group = [&content_groups](const std::string& group) {
    return base::Contains(content_groups, group,
                          [](const auto& other) { return other.first; });
  };
  EXPECT_TRUE(has_group(kTopNewsChannel));
  EXPECT_TRUE(has_group(kFooChannel));
  EXPECT_TRUE(has_group(kPublisher1));
}

TEST_F(BraveNewsFeedGenerationInfoTest,
       RemovingArticlesUpdatesEligibleContentGroups) {
  auto [locale, feed_items, publishers, signals, topics] = MockFetchedData();
  feed_items.push_back(MakeArticleItem(kPublisher1));
  feed_items.push_back(MakeArticleItem(kPublisher2));
  feed_items.push_back(MakeArticleItem(kPublisherDisabled));
  FeedGenerationInfo info(SubscriptionsSnapshot(), "en_NZ", feed_items,
                          publishers, {kTopNewsChannel, kFooChannel}, signals,
                          {}, topics);

  EXPECT_EQ(3u, info.GetEligibleContentGroups().size());

  // Removing the article from publisher1 should remove everything from p1 and
  // kTopNews.
  PickArticles pickP1 = base::BindRepeating(
      [](const ArticleInfos& articles) -> std::optional<size_t> {
        for (size_t i = 0; i < articles.size(); ++i) {
          if (std::get<0>(articles[i])->publisher_id == kPublisher1) {
            return std::make_optional(i);
          }
        }
        return std::nullopt;
      });
  info.PickAndConsume(pickP1);
  EXPECT_EQ(1u, info.GetEligibleContentGroups().size());
}

TEST_F(BraveNewsFeedGenerationInfoTest,
       RemovingArticlesUpdatesEligibleChannels) {
  auto [locale, feed_items, publishers, signals, topics] = MockFetchedData();
  feed_items.push_back(MakeArticleItem(kPublisher1));
  feed_items.push_back(MakeArticleItem(kPublisher2));
  feed_items.push_back(MakeArticleItem(kPublisherDisabled));
  FeedGenerationInfo info(SubscriptionsSnapshot(), "en_NZ", feed_items,
                          publishers, {kTopNewsChannel, kFooChannel}, signals,
                          {}, topics);

  EXPECT_EQ(3u, info.GetEligibleContentGroups().size());
  EXPECT_EQ(2u, info.EligibleChannels().size());

  // Pick top news
  PickArticles pick_top_news = base::BindRepeating(
      [](const ArticleInfos& articles) -> std::optional<size_t> {
        for (size_t i = 0; i < articles.size(); ++i) {
          if (base::Contains(std::get<1>(articles[i]).channels,
                             kTopNewsChannel)) {
            return std::make_optional(i);
          }
        }
        return std::nullopt;
      });
  info.PickAndConsume(pick_top_news);
  EXPECT_EQ(1u, info.GetEligibleContentGroups().size());

  auto channels = info.EligibleChannels();
  EXPECT_EQ(1u, channels.size());
  EXPECT_TRUE(base::Contains(channels, kFooChannel));
}

TEST(BraveNewsFeedSampling, GetArticleInfosSkipsNull) {
  auto [locale, items, publishers, signals, topics] = MockFetchedData();
  items.push_back(nullptr);

  EXPECT_EQ(
      0u, GetArticleInfosForTesting(locale, items, publishers, signals).size());
}

TEST(BraveNewsFeedSampling, GetArticleInfosSkipsNonArticles) {
  auto [locale, items, publishers, signals, topics] = MockFetchedData();

  items.push_back(MakeArticleItem("one"));
  items.push_back(MakeArticleItem("two"));
  items.push_back(mojom::FeedItem::NewDeal(mojom::Deal::New()));

  EXPECT_EQ(
      2u, GetArticleInfosForTesting(locale, items, publishers, signals).size());
}

TEST(BraveNewsFeedSampling, GetArticleInfosDuplicatesExcluded) {
  auto [locale, items, publishers, signals, topics] = MockFetchedData();

  auto item = MakeArticleItem("one");
  items.push_back(item->Clone());
  items.push_back(std::move(item));

  EXPECT_EQ(
      1u, GetArticleInfosForTesting(locale, items, publishers, signals).size());
}
TEST(BraveNewsFeedSampling, GetArticleInfosUnknownPublishersSkipped) {
  auto [locale, items, publishers, signals, topics] = MockFetchedData();

  items.push_back(MakeArticleItem("one"));
  items.push_back(MakeArticleItem("not-a-real-publisher"));

  EXPECT_EQ(
      1u, GetArticleInfosForTesting(locale, items, publishers, signals).size());
}

TEST(BraveNewsFeedSampling, GetArticleInfosDisabledArticlesExcluded) {
  auto [locale, items, publishers, signals, topics] = MockFetchedData();

  items.push_back(MakeArticleItem("disabled"));
  items.push_back(MakeArticleItem("one"));

  EXPECT_EQ(
      1u, GetArticleInfosForTesting(locale, items, publishers, signals).size());
}

TEST(BraveNewsFeedSampling, GetArticleInfosUsesCorrectSignals) {
  auto [locale, items, publishers, signals, topics] = MockFetchedData();

  items.push_back(MakeArticleItem("disabled"));
  items.push_back(MakeArticleItem("one"));
  items.push_back(MakeArticleItem("two"));
  items.push_back(MakeArticleItem("not-a-real-publisher"));

  auto infos = GetArticleInfosForTesting(locale, items, publishers, signals);
  EXPECT_EQ(2u, infos.size());

  auto& [article0, weight0] = infos.at(0);
  EXPECT_EQ("one", article0->publisher_id);
  EXPECT_TRUE(weight0.visited);
  EXPECT_TRUE(weight0.subscribed);

  auto& [article1, weight1] = infos.at(1);
  EXPECT_EQ("two", article1->publisher_id);
  EXPECT_FALSE(weight1.visited);
  EXPECT_TRUE(weight1.subscribed);
}

}  // namespace brave_news
