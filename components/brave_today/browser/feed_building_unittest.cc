// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include <algorithm>
#include <iterator>
#include <memory>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/containers/flat_map.h"
#include "brave/components/brave_today/browser/feed_building.h"
#include "brave/components/brave_today/common/brave_news.mojom-forward.h"
#include "brave/components/brave_today/common/brave_news.mojom-shared.h"
#include "brave/components/brave_today/common/brave_news.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_news {

namespace {

using Publishers = base::flat_map<std::string, mojom::PublisherPtr>;

std::string GetFeedJson() {
  // One item has a higher score, but a matching history domain - it should
  // appear earlier.
  // First item has a higher score, so it should appear later.
  return R"([
        {
          "category": "Technology",
          "publish_time": "2021-09-01 07:01:28",
          "url": "https://www.example.com/an-article/",
          "title": "Expecting ThirdLogitech built Bolt to make wireless mice and keyboards work better",
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
          "category": "Technology",
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
          "score": 13.91160989810695
        }
      ]
    )";
}

void PopulatePublishers(Publishers* publisher_list) {
  auto publisher1 =
      mojom::Publisher::New("111", "First Publisher", "Top News", true,
                            mojom::UserEnabled::NOT_MODIFIED);
  auto publisher2 =
      mojom::Publisher::New("222", "Second Publisher", "Top News", true,
                            mojom::UserEnabled::NOT_MODIFIED);
  auto publisher3 =
      mojom::Publisher::New("333", "Third Publisher", "Top News", true,
                            mojom::UserEnabled::NOT_MODIFIED);
  publisher_list->insert_or_assign(publisher1->publisher_id,
                                   std::move(publisher1));
  publisher_list->insert_or_assign(publisher2->publisher_id,
                                   std::move(publisher2));
  publisher_list->insert_or_assign(publisher3->publisher_id,
                                   std::move(publisher3));
}

}  // namespace

TEST(BraveNewsFeedBuilding, BuildFeed) {
  Publishers publisher_list;
  PopulatePublishers(&publisher_list);

  std::unordered_set<std::string> history_hosts = {"www.espn.com"};

  mojom::Feed feed;
  ASSERT_TRUE(BuildFeed(GetFeedJson(), history_hosts, &publisher_list, &feed));
  ASSERT_EQ(feed.pages.size(), 1u);
  // Validate featured article is top news
  ASSERT_TRUE(feed.featured_item->is_article());
  ASSERT_EQ(feed.featured_item->get_article()->data->url.spec(),
            "https://foreignpolicy.com/2021/09/01/"
            "africa-youth-protests-senegal-sudan-ghana-eswatini/");
  // Validate sorted by score descending
  ASSERT_GE(feed.pages[0]->items.size(), 3u);
  // Because we cannot access a flat list, then select the items from each card
  // (some cards have 1 item, some have 2, etc). If the page_content_order
  // changes, then also change here which items we access in which order.
  ASSERT_EQ(feed.pages[0]->items[0]->items.size(), 1u);
  ASSERT_EQ(feed.pages[0]->items[0]->items[0]->get_article()->data->url,
            "https://www.espn.com/soccer/blog-transfer-talk/story/4465789/"
            "live-transfer-deadline-day-will-real-madrid-land-psg-star-mbappe");
  ASSERT_EQ(feed.pages[0]->items[1]->items.size(), 1u);
  ASSERT_EQ(feed.pages[0]->items[1]->items[0]->get_article()->data->url,
            "https://www.digitaltrends.com/computing/"
            "logi-bolt-secure-wireless-connectivity/");
  ASSERT_EQ(feed.pages[0]->items[2]->items.size(), 1u);
  ASSERT_EQ(feed.pages[0]->items[2]->items[0]->get_article()->data->url,
            "https://www.example.com/an-article/");
}

TEST(BraveNewsFeedBuilding, RemovesDefaultOffItems) {
  Publishers publisher_list;
  PopulatePublishers(&publisher_list);

  // Set a publisher to default-off, it's items should not appear in feed
  std::string publisher_id_to_hide = "333";
  publisher_list.at(publisher_id_to_hide)->is_enabled = false;

  std::vector<mojom::FeedItemPtr> feed_items;
  ASSERT_TRUE(
      ParseFeedItemsToDisplay(GetFeedJson(), &publisher_list, &feed_items));
  auto match = std::find_if(
      feed_items.begin(), feed_items.end(),
      [publisher_id_to_hide](mojom::FeedItemPtr const& item) {
        return item->get_article()->data->publisher_id == publisher_id_to_hide;
      });
  auto found = (match != feed_items.end());
  ASSERT_EQ(found, false);
}

TEST(BraveNewsFeedBuilding, RemovesUserDisabledItems) {
  Publishers publisher_list;
  PopulatePublishers(&publisher_list);

  // Set a publisher to default-on, but user-off
  std::string publisher_id_to_hide = "333";
  publisher_list.at(publisher_id_to_hide)->is_enabled = true;
  publisher_list.at(publisher_id_to_hide)->user_enabled_status =
      mojom::UserEnabled::DISABLED;

  std::vector<mojom::FeedItemPtr> feed_items;
  ASSERT_TRUE(
      ParseFeedItemsToDisplay(GetFeedJson(), &publisher_list, &feed_items));
  auto match = std::find_if(
      feed_items.begin(), feed_items.end(),
      [publisher_id_to_hide](mojom::FeedItemPtr const& item) {
        return item->get_article()->data->publisher_id == publisher_id_to_hide;
      });
  auto found = (match != feed_items.end());
  ASSERT_EQ(found, false);
}

TEST(BraveNewsFeedBuilding, IncludesUserEnabledItems) {
  Publishers publisher_list;
  PopulatePublishers(&publisher_list);

  // Set a publisher to default-off, but user-on
  std::string publisher_id_to_hide = "333";
  publisher_list.at(publisher_id_to_hide)->is_enabled = false;
  publisher_list.at(publisher_id_to_hide)->user_enabled_status =
      mojom::UserEnabled::ENABLED;

  std::vector<mojom::FeedItemPtr> feed_items;
  ASSERT_TRUE(
      ParseFeedItemsToDisplay(GetFeedJson(), &publisher_list, &feed_items));
  auto match = std::find_if(
      feed_items.begin(), feed_items.end(),
      [publisher_id_to_hide](mojom::FeedItemPtr const& item) {
        return item->get_article()->data->publisher_id == publisher_id_to_hide;
      });
  auto found = (match != feed_items.end());
  ASSERT_EQ(found, true);
}

}  // namespace brave_news
