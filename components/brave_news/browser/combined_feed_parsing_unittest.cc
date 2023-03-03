// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <string>
#include <utility>
#include <vector>

#include "base/values.h"
#include "brave/components/brave_news/browser/combined_feed_parsing.h"
#include "brave/components/brave_news/common/brave_news.mojom-shared.h"
#include "brave/components/brave_news/common/brave_news.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_news {

namespace {

base::Value GetItem(std::string url) {
  base::Value::Dict item;
  item.Set("content_type", base::Value("article"));
  item.Set("url", base::Value(url));
  item.Set("padded_img", base::Value("https://example.com/img.jpg.pad"));
  item.Set("publisher_id", base::Value("Id1"));
  item.Set("publisher_name", base::Value("Publisher1"));
  item.Set("title", base::Value("Title"));
  item.Set("description", base::Value("Description"));
  item.Set("category", base::Value("Category1"));
  item.Set("score", base::Value(1.0));
  item.Set("publish_time", base::Value("2022-11-07 09:00:09"));
  return base::Value(std::move(item));
}

}  // namespace

TEST(BraveNewCombinedFeedParsing, Success) {
  // Create an entry which should be valid as a Brave News item
  auto item = GetItem("https://www.hello.com");
  base::Value::List list;
  list.Append(std::move(item));
  base::Value json_value = base::Value(std::move(list));

  std::vector<mojom::FeedItemPtr> feed_items =
      ParseFeedItems(std::move(json_value));
  // The single item should be successfully parsed to a FeedItem
  EXPECT_EQ(feed_items.size(), 1u);
}

TEST(BraveNewCombinedFeedParsing, FailBadProtocol) {
  // Create an entry which should be invalid as a Brave News item
  // A chrome: protocol should not be allowed
  auto item = GetItem("chrome://settings");
  base::Value::List list;
  list.Append(std::move(item));
  base::Value json_value = base::Value(std::move(list));

  std::vector<mojom::FeedItemPtr> feed_items =
      ParseFeedItems(std::move(json_value));
  // The single item should not be parsed to a FeedItem
  EXPECT_EQ(feed_items.size(), 0u);
}

}  // namespace brave_news
