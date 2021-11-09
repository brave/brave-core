// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include <memory>
#include <utility>
#include <vector>

#include "base/containers/flat_map.h"
#include "brave/components/brave_today/browser/publishers_parsing.h"
#include "brave/components/brave_today/common/brave_news.mojom-forward.h"
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
        "enabled": false
      },
      {
        "publisher_id": "222",
        "publisher_name": "Test Publisher 2",
        "category": "Sports",
        "enabled": true
      },
      {
        "publisher_id": "333",
        "publisher_name": "Test Publisher 3",
        "category": "Design",
        "enabled": true
      }
    ]
  )");
  base::flat_map<std::string, mojom::PublisherPtr> publisher_list;
  ASSERT_TRUE(ParsePublisherList(json, &publisher_list));
  ASSERT_EQ(publisher_list.size(), 3UL);

  ASSERT_TRUE(publisher_list.contains("111"));
  auto first_opt = publisher_list.find("111");
  ASSERT_NE(first_opt, publisher_list.end());
  // auto first = first_opt->second;

  ASSERT_EQ(first_opt->second->publisher_id, "111");
  ASSERT_EQ(first_opt->second->publisher_name, "Test Publisher 1");

  ASSERT_TRUE(publisher_list.contains("222"));
  ASSERT_TRUE(publisher_list.contains("333"));
  ASSERT_FALSE(publisher_list.contains("444"));
}

}  // namespace brave_news
