// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/publishers_parsing.h"

#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/test/values_test_util.h"
#include "brave/components/brave_news/common/brave_news.mojom-forward.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_news {

TEST(BraveNewsPublisherParsing, ParsePublisherList) {
  // Test that we parse expected remote publisher JSON
  std::string json(R"(
    [
      {
        "publisher_id": "111",
        "publisher_name": "Test Publisher 1",
        "category": "Tech",
        "enabled": false,
        "site_url": "https://one.example.com",
        "feed_url": "https://one.example.com/feed"
      },
      {
        "publisher_id": "222",
        "publisher_name": "Test Publisher 2",
        "category": "Sports",
        "enabled": true,
        "site_url": "https://two.example.com",
        "feed_url": "https://two.example.com/feed"
      },
      {
        "publisher_id": "333",
        "publisher_name": "Test Publisher 3",
        "category": "Design",
        "enabled": true,
        "site_url": "https://three.example.com",
        "feed_url": "https://three.example.com/feed"
      }
    ]
  )");
  std::optional<Publishers> publisher_list =
      ParseCombinedPublisherList(base::test::ParseJson(json));
  ASSERT_TRUE(publisher_list);
  ASSERT_EQ(publisher_list->size(), 3UL);

  ASSERT_TRUE(publisher_list->contains("111"));
  auto first_opt = publisher_list->find("111");
  ASSERT_NE(first_opt, publisher_list->end());

  ASSERT_EQ(first_opt->second->publisher_id, "111");
  ASSERT_EQ(first_opt->second->publisher_name, "Test Publisher 1");

  ASSERT_TRUE(publisher_list->contains("222"));
  ASSERT_TRUE(publisher_list->contains("333"));
  ASSERT_FALSE(publisher_list->contains("444"));
}

TEST(BraveNewsPublisherParsing, PublisherListWithNoneValuesInOptionalFields) {
  // Test that we parse expected remote publisher JSON
  const char json[] = (R"(
    [
      {
        "publisher_id": "111",
        "publisher_name": "Test Publisher 1",
        "category": "Tech",
        "enabled": false,
        "site_url": "https://one.example.com",
        "feed_url": "https://one.example.com/feed"
      },
      {
        "publisher_id": "222",
        "publisher_name": "Test Publisher 2",
        "category": "Sports",
        "enabled": true,
        "site_url": "https://two.example.com",
        "feed_url": "https://two.example.com/feed",
        "favicon_url": null,
        "cover_url": null,
        "background_color": null
      },
      {
        "publisher_id": "333",
        "publisher_name": "Test Publisher 3",
        "category": "Design",
        "enabled": true,
        "site_url": "https://three.example.com",
        "feed_url": "https://three.example.com/feed"
      }
    ]
  )");
  std::optional<Publishers> publisher_list =
      ParseCombinedPublisherList(base::test::ParseJson(json));
  ASSERT_TRUE(publisher_list);
  ASSERT_EQ(publisher_list->size(), 3UL);
}

TEST(BraveNewsPublisherParsing, ChannelsAreMigrated) {
  // Test that we parse expected remote publisher JSON
  const char json[] = (R"(
    [
      {
        "publisher_id": "111",
        "publisher_name": "Test Publisher 1",
        "category": "",
        "locales": [
          {
            "locale": "en_US",
            "channels": [
              "Tech News",
              "Tech Reviews",
              "Technology"
            ]
          },
          {
            "locale": "en_NZ",
            "channels": [
              "Sport",
              "Stuff"
            ]
          },
          {
            "locale": "en_AU",
            "channels": [
              "Celebrity News"
            ]
          }
        ],
        "enabled": false,
        "site_url": "https://one.example.com",
        "feed_url": "https://one.example.com/feed"
      }
    ]
  )");

  std::optional<Publishers> publisher_list =
      ParseCombinedPublisherList(base::test::ParseJson(json));
  ASSERT_TRUE(publisher_list);
  ASSERT_EQ(publisher_list->size(), 1ul);

  auto& publisher = (*publisher_list)["111"];
  ASSERT_EQ(3u, publisher->locales.size());

  auto& en_us = publisher->locales[0];
  ASSERT_EQ("en_US", en_us->locale);
  ASSERT_EQ(1u, en_us->channels.size());
  EXPECT_EQ("Technology", en_us->channels[0]);

  auto& en_nz = publisher->locales[1];
  ASSERT_EQ("en_NZ", en_nz->locale);
  ASSERT_EQ(2u, en_nz->channels.size());
  EXPECT_EQ("Sports", en_nz->channels[0]);
  EXPECT_EQ("Stuff", en_nz->channels[1]);

  auto& en_au = publisher->locales[2];
  ASSERT_EQ("en_AU", en_au->locale);
  ASSERT_EQ(1u, en_au->channels.size());
  EXPECT_EQ("Celebrities", en_au->channels[0]);
}

}  // namespace brave_news
