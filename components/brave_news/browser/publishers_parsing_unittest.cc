// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <memory>
#include <utility>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/test/values_test_util.h"
#include "brave/components/brave_news/browser/publishers_parsing.h"
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
  absl::optional<Publishers> publisher_list =
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
  absl::optional<Publishers> publisher_list =
      ParseCombinedPublisherList(base::test::ParseJson(json));
  ASSERT_TRUE(publisher_list);
  ASSERT_EQ(publisher_list->size(), 3UL);
}

}  // namespace brave_news
