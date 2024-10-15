// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/feed_building.h"

#include <algorithm>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/containers/extend.h"
#include "base/containers/flat_map.h"
#include "base/test/values_test_util.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/brave_news/browser/brave_news_pref_manager.h"
#include "brave/components/brave_news/browser/channels_controller.h"
#include "brave/components/brave_news/browser/combined_feed_parsing.h"
#include "brave/components/brave_news/common/brave_news.mojom.h"
#include "brave/components/brave_news/common/pref_names.h"
#include "brave/components/brave_news/common/subscriptions_snapshot.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/mojom/url.mojom.h"

namespace brave_news {

namespace {

using Publishers = base::flat_map<std::string, mojom::PublisherPtr>;

base::Value GetFeedJson() {
  // One item has a higher score, but a matching history domain - it should
  // appear earlier.
  // First item has a higher score, so it should appear later.
  return base::test::ParseJson(R"([
        {
          "category": "Technology",
          "publish_time": "2021-09-01 07:01:28",
          "url": "https://www.example.com/an-article/",
          "title": "Expecting Third Logitech built Bolt to make wireless mice and keyboards work better",
          "description": "Built on top of Bluetooth Low Energy, Logi Bolt is designed to reliably and securely connect wireless mice and keyboard to business PCs.",
          "content_type": "article",
          "publisher_id": "222",
          "publisher_name": "Digital Trends",
          "creative_instance_id": "",
          "url_hash": "523b9f2091474c2a082c06ec17965f8c2392f871917407228bbeb51d8a55d6be",
          "padded_img": "https://pcdn.brave.com/brave-today/cache/052e832456e00a3cee51c68eee206fe71c32cba35d5e53dee2777dd132e01364.jpg.pad",
          "score": 13.93160989810695
        },
        {
          "category": "",
          "publish_time": "2020-09-01 07:01:28",
          "url": "https://fourth.example.com/an-article/",
          "title": "Expecting Fourth Logitech built Bolt to make wireless mice and keyboards work better",
          "description": "Built on top of Bluetooth Low Energy, Logi Bolt is designed to reliably and securely connect wireless mice and keyboard to business PCs.",
          "content_type": "article",
          "publisher_id": "444",
          "publisher_name": "Fourth Publisher",
          "creative_instance_id": "",
          "url_hash": "523b9f2091474c2a082c06ec17965f8c2392f871917407228bbeb51d8a55d6be",
          "padded_img": "https://pcdn.brave.com/brave-today/cache/052e832456e00a3cee51c68eee206fe71c32cba35d5e53dee2777dd132e01364.jpg.pad",
          "score": 22.93160989810695
        },
        {
          "category": "Technology",
          "publish_time": "2021-09-01 07:04:32",
          "url": "https://www.espn.com/soccer/blog-transfer-talk/story/4465789/live-transfer-deadline-day-will-real-madrid-land-psg-star-mbappe",
          "title": "Expecting First Transfer Talk: How a busy Deadline Day unfolded",
          "description": "The transfer window is closed and Saul Niguez is on his way to Chelsea, while Antoine Griezmann is set to go back to Atletico Madrid on loan from Barcelona. Check out all the deals from a busy day.",
          "content_type": "article",
          "publisher_id": "111",
          "publisher_name": "ESPN - Football",
          "creative_instance_id": "",
          "url_hash": "7bb5d8b3e2eee9d317f0568dcb094850fdf2862b2ed6d583c62b2245ea507ab8",
          "padded_img": "https://pcdn.brave.com/brave-today/cache/85fb134433369025b46b861a00408e61223678f55620612d980533fa6ce0a815.jpg.pad",
          "score": 14.525910905005045
        },
        {
          "category": "Top News",
          "publish_time": "2021-09-01 07:00:58",
          "url": "https://foreignpolicy.com/2021/09/01/africa-youth-protests-senegal-sudan-ghana-eswatini/",
          "title": "Expecting Featured Africa\u2019s Disappointed Demographic",
          "description": "Young people across the continent have been hit hard by the pandemic, lockdowns, and economic stagnation\u2014but their protests have largely been ignored by elderly elites.",
          "content_type": "article",
          "publisher_id": "333",
          "publisher_name": "Foreign Policy",
          "creative_instance_id": "",
          "url_hash": "9aaa370ed4c2888bc6603404dcc44ed1125d3347101873798d2ec8a0a9c424b1",
          "padded_img": "https://pcdn.brave.com/brave-today/cache/4f7ab8aef2ffb518bc4226d2c50487b6b9bde5f781579288b5b3dde92847db7a.jpg.pad",
          "score": 13.96799592432192
        },
        {
          "category": "Top News",
          "publish_time": "2021-09-01 07:01:28",
          "url": "https://www.digitaltrends.com/computing/logi-bolt-secure-wireless-connectivity/",
          "title": "Expecting Second Logitech built Bolt to make wireless mice and keyboards work better",
          "description": "Built on top of Bluetooth Low Energy, Logi Bolt is designed to reliably and securely connect wireless mice and keyboard to business PCs.",
          "content_type": "article",
          "publisher_id": "222",
          "publisher_name": "Digital Trends",
          "creative_instance_id": "",
          "url_hash": "523b9f2091474c2a082c06ec17965f8c2392f871917407228bbeb51d8a55d6be",
          "padded_img": "https://pcdn.brave.com/brave-today/cache/052e832456e00a3cee51c68eee206fe71c32cba35d5e53dee2777dd132e01364.jpg.pad",
          "score": 13.97160989810695
        }
      ]
    )");
}

std::vector<mojom::LocaleInfoPtr> CreateLocales(
    const std::vector<std::string>& locales,
    const std::vector<std::string>& channels) {
  std::vector<mojom::LocaleInfoPtr> result;
  for (const auto& locale : locales) {
    auto info = mojom::LocaleInfo::New();
    info->locale = locale;
    info->channels = channels;
    result.push_back(std::move(info));
  }
  return result;
}

void PopulatePublishers(Publishers* publisher_list) {
  auto publisher1 = mojom::Publisher::New(
      "111", mojom::PublisherType::COMBINED_SOURCE, "First Publisher",
      "Top News", true, CreateLocales({"en_US"}, {"Top News", "Top Sources"}),
      GURL("https://www.example.com"), std::nullopt, std::nullopt, std::nullopt,
      GURL("https://first-publisher.com/feed.xml"),
      mojom::UserEnabled::NOT_MODIFIED);
  auto publisher2 = mojom::Publisher::New(
      "222", mojom::PublisherType::COMBINED_SOURCE, "Second Publisher",
      "Top News", true, CreateLocales({"en_US"}, {"Top News", "Top Sources"}),
      GURL("https://www.example.com"), std::nullopt, std::nullopt, std::nullopt,
      GURL("https://second-publisher.com/feed.xml"),
      mojom::UserEnabled::NOT_MODIFIED);
  auto publisher3 = mojom::Publisher::New(
      "333", mojom::PublisherType::COMBINED_SOURCE, "Third Publisher",
      "Top News", true, CreateLocales({"en_US"}, {"Top News"}),
      GURL("https://www.example.com"), std::nullopt, std::nullopt, std::nullopt,
      GURL("https://third-publisher.com/feed.xml"),
      mojom::UserEnabled::NOT_MODIFIED);

  auto publisher4 = mojom::Publisher::New(
      "444", mojom::PublisherType::COMBINED_SOURCE, "Fourth Publisher", "f",
      false, CreateLocales({}, {}), GURL("https://fourth.example.com"),
      std::nullopt, std::nullopt, std::nullopt,
      GURL("https://fourth-publisher.com/feed.xml"),
      mojom::UserEnabled::ENABLED);

  publisher_list->insert_or_assign(publisher1->publisher_id,
                                   std::move(publisher1));
  publisher_list->insert_or_assign(publisher2->publisher_id,
                                   std::move(publisher2));
  publisher_list->insert_or_assign(publisher3->publisher_id,
                                   std::move(publisher3));
  publisher_list->insert_or_assign(publisher4->publisher_id,
                                   std::move(publisher4));
}

}  // namespace

class BraveNewsFeedBuildingTest : public testing::Test {
 public:
  BraveNewsFeedBuildingTest() {
    prefs::RegisterProfilePrefs(pref_service_.registry());

    pref_manager_ = std::make_unique<BraveNewsPrefManager>(pref_service_);
  }

  BraveNewsFeedBuildingTest(const BraveNewsFeedBuildingTest&) = delete;
  BraveNewsFeedBuildingTest& operator=(const BraveNewsFeedBuildingTest&) =
      delete;
  ~BraveNewsFeedBuildingTest() override = default;

 protected:
  sync_preferences::TestingPrefServiceSyncable pref_service_;

  std::unique_ptr<BraveNewsPrefManager> pref_manager_;
};

TEST_F(BraveNewsFeedBuildingTest, BuildFeed) {
  pref_manager_->SetChannelSubscribed("en_US", "Top Sources", true);

  Publishers publisher_list;
  PopulatePublishers(&publisher_list);

  std::unordered_set<std::string> history_hosts = {"www.espn.com"};

  std::vector<mojom::FeedItemPtr> feed_items = ParseFeedItems(GetFeedJson());

  mojom::Feed feed;

  ASSERT_TRUE(BuildFeed(feed_items, history_hosts, &publisher_list, &feed,
                        pref_manager_->GetSubscriptions()));
  ASSERT_EQ(feed.pages.size(), 1u);
  // Validate featured article is top news
  ASSERT_TRUE(feed.featured_item->is_article());
  ASSERT_EQ(feed.featured_item->get_article()->data->url.spec(),
            "https://www.digitaltrends.com/computing/"
            "logi-bolt-secure-wireless-connectivity/");
  // Validate sorted by score descending
  ASSERT_GE(feed.pages[0]->items.size(), 3u);
  // Because we cannot access a flat list, then select the items from each card
  // (some cards have 1 item, some have 2, etc). If the page_content_order
  // changes, then also change here which items we access in which order.
  ASSERT_EQ(feed.pages[0]->items[0]->items.size(), 1u);
  ASSERT_EQ(feed.pages[0]->items[0]->items[0]->get_article()->data->url,
            "https://www.espn.com/soccer/blog-transfer-talk/story/4465789/"
            "live-transfer-deadline-day-will-real-madrid-land-psg-star-mbappe");
  // "444" publisher item should be ahead of others since although it has a
  // worse score, it is directly followed and so its score gets bumped to be
  // more highly ranked.
  ASSERT_EQ(feed.pages[0]->items[1]->items.size(), 1u);
  ASSERT_EQ(feed.pages[0]->items[1]->items[0]->get_article()->data->url,
            "https://fourth.example.com/an-article/");
  ASSERT_EQ(feed.pages[0]->items[2]->items.size(), 1u);
  ASSERT_EQ(feed.pages[0]->items[2]->items[0]->get_article()->data->url,
            "https://www.example.com/an-article/");
  auto direct_follow_score =
      feed.pages[0]->items[1]->items[0]->get_article()->data->score;
  auto channel_follow_score =
      feed.pages[0]->items[2]->items[0]->get_article()->data->score;
  // Lower score means higher ranked
  ASSERT_LT(direct_follow_score, channel_follow_score);
}

TEST_F(BraveNewsFeedBuildingTest, DirectFeedsShouldAlwaysBeDisplayed) {
  Channels channels;
  Publishers publisher_list;
  PopulatePublishers(&publisher_list);
  auto* publisher = publisher_list["111"].get();
  publisher->type = mojom::PublisherType::DIRECT_SOURCE;
  publisher->user_enabled_status = mojom::UserEnabled::NOT_MODIFIED;

  auto feed_item = mojom::FeedItem::NewArticle(mojom::Article::New(
      mojom::FeedItemMetadata::New(
          "Technology", std::vector<std::string>(), base::Time::Now(), "Title",
          "Description", GURL("https://example.com/article"),
          "7bb5d8b3e2eee9d317f0568dcb094850fdf2862b2ed6d583c62b2245ea507ab8",
          mojom::Image::NewPaddedImageUrl(
              GURL("https://example.com/article/image")),
          publisher->publisher_id, "Source", 10, 0, "a minute ago"),
      false));
  EXPECT_TRUE(ShouldDisplayFeedItem(feed_item, &publisher_list, channels));

  publisher->locales = std::vector<mojom::LocaleInfoPtr>();
  EXPECT_TRUE(ShouldDisplayFeedItem(feed_item, &publisher_list, channels));
}

TEST_F(BraveNewsFeedBuildingTest, RemovesUserDisabledItems) {
  Publishers publisher_list;
  PopulatePublishers(&publisher_list);
  std::unordered_set<std::string> history_hosts = {};

  // Set a publisher to default-on, but user-off
  std::string publisher_id_to_hide = "333";
  publisher_list.at(publisher_id_to_hide)->is_enabled = true;
  publisher_list.at(publisher_id_to_hide)->user_enabled_status =
      mojom::UserEnabled::DISABLED;

  auto feed_item = mojom::FeedItem::NewArticle(mojom::Article::New(
      mojom::FeedItemMetadata::New(
          "Technology", std::vector<std::string>(), base::Time::Now(),
          "Expecting First Transfer Talk: How a busy Deadline Day unfolded",
          "The transfer window is closed and Saul Niguez is on his way to "
          "Chelsea, while Antoine Griezmann is set to go back to Atletico "
          "Madrid on loan from Barcelona. Check out all the deals from a busy "
          "day.",
          GURL("https://www.espn.com/soccer/blog-transfer-talk/story/4465789/"
               "live-transfer-deadline-day-will-real-madrid-land-psg-star-"
               "mbappe"),
          "7bb5d8b3e2eee9d317f0568dcb094850fdf2862b2ed6d583c62b2245ea507ab8",
          mojom::Image::NewPaddedImageUrl(
              GURL("https://pcdn.brave.com/brave-today/cache/"
                   "85fb134433369025b46b861a00408e61223678f55620612d980533fa6ce"
                   "0a815.jpg.pad")),
          publisher_id_to_hide, "ESPN - Football", 14.525910905005045, 0,
          "a minute ago"),
      false));

  Channels channels;
  ASSERT_FALSE(ShouldDisplayFeedItem(feed_item, &publisher_list, channels));
}

TEST_F(BraveNewsFeedBuildingTest, IncludesUserEnabledItems) {
  Publishers publisher_list;

  PopulatePublishers(&publisher_list);
  std::unordered_set<std::string> history_hosts = {};

  // Set a publisher to default-off, but user-on
  std::string publisher_id_to_hide = "333";
  publisher_list.at(publisher_id_to_hide)->is_enabled = false;
  publisher_list.at(publisher_id_to_hide)->user_enabled_status =
      mojom::UserEnabled::ENABLED;

  auto feed_item = mojom::FeedItem::NewArticle(mojom::Article::New(
      mojom::FeedItemMetadata::New(
          "Technology", std::vector<std::string>(), base::Time::Now(),
          "Expecting First Transfer Talk: How a busy Deadline Day unfolded",
          "The transfer window is closed and Saul Niguez is on his way to "
          "Chelsea, while Antoine Griezmann is set to go back to Atletico "
          "Madrid on loan from Barcelona. Check out all the deals from a busy "
          "day.",
          GURL("https://www.espn.com/soccer/blog-transfer-talk/story/4465789/"
               "live-transfer-deadline-day-will-real-madrid-land-psg-star-"
               "mbappe"),
          "7bb5d8b3e2eee9d317f0568dcb094850fdf2862b2ed6d583c62b2245ea507ab8",
          mojom::Image::NewPaddedImageUrl(
              GURL("https://pcdn.brave.com/brave-today/cache/"
                   "85fb134433369025b46b861a00408e61223678f55620612d980533fa6ce"
                   "0a815.jpg.pad")),
          publisher_id_to_hide, "ESPN - Football", 14.525910905005045, 0,
          "a minute ago"),
      false));

  Channels channels;
  ASSERT_TRUE(ShouldDisplayFeedItem(feed_item, &publisher_list, channels));
}

TEST_F(BraveNewsFeedBuildingTest, ChannelIsUsed) {
  Publishers publisher_list;
  PopulatePublishers(&publisher_list);
  auto* publisher = publisher_list["111"].get();

  Channels channels;
  channels.insert(
      {"Top News", brave_news::mojom::Channel::New(
                       "Top News", std::vector<std::string>{"en_US"})});
  auto* channel = channels["Top News"].get();

  auto feed_item = mojom::FeedItem::NewArticle(mojom::Article::New(
      mojom::FeedItemMetadata::New(
          "Technology", std::vector<std::string>(), base::Time::Now(), "Title",
          "Description", GURL("https://example.com/article"),
          "7bb5d8b3e2eee9d317f0568dcb094850fdf2862b2ed6d583c62b2245ea507ab8",
          mojom::Image::NewPaddedImageUrl(
              GURL("https://example.com/article/image")),
          publisher->publisher_id, "Source", 10, 0, "a minute ago"),
      false));

  // Publisher: NOT_MODIFIED, Channel: Subscribed, Should display.
  EXPECT_TRUE(ShouldDisplayFeedItem(feed_item, &publisher_list, channels));

  // Publisher: NOT_MODIFIED, Channel: Not subscribed in any locale, Should not
  // display.
  channel->subscribed_locales = {};
  EXPECT_FALSE(ShouldDisplayFeedItem(feed_item, &publisher_list, channels));

  // Publisher: ENABLED, Channel: Not subscribed, Should display.
  publisher->user_enabled_status = mojom::UserEnabled::ENABLED;
  EXPECT_TRUE(ShouldDisplayFeedItem(feed_item, &publisher_list, channels));

  // Publisher: Disabled, Channel: Not subscribed, Should not display.
  publisher->user_enabled_status = mojom::UserEnabled::DISABLED;
  EXPECT_FALSE(ShouldDisplayFeedItem(feed_item, &publisher_list, channels));

  // Publisher: DISABLED, Channel: Subscribed in en_US, Should not display.
  channel->subscribed_locales = {"en_US"};
  EXPECT_FALSE(ShouldDisplayFeedItem(feed_item, &publisher_list, channels));
}

TEST_F(BraveNewsFeedBuildingTest, DuplicateItemsAreNotIncluded) {
  pref_manager_->SetChannelSubscribed("en_US", "Top Sources", true);

  Publishers publisher_list;
  PopulatePublishers(&publisher_list);

  std::unordered_set<std::string> history_hosts = {"www.espn.com"};

  // Parse the feed items twice so we get two copies of everything.
  std::vector<mojom::FeedItemPtr> feed_items = ParseFeedItems(GetFeedJson());
  base::Extend(feed_items, ParseFeedItems(GetFeedJson()));

  mojom::Feed feed;

  ASSERT_TRUE(BuildFeed(feed_items, history_hosts, &publisher_list, &feed,
                        pref_manager_->GetSubscriptions()));
  ASSERT_EQ(feed.pages.size(), 1u);
  ASSERT_EQ(feed.pages[0]->items.size(), 18u);
}

}  // namespace brave_news
