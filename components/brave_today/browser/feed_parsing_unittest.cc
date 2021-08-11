// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include <memory>
#include <utility>
#include <vector>

#include "base/containers/flat_map.h"
#include "brave/components/brave_today/browser/feed_parsing.h"
#include "brave/components/brave_today/common/brave_news.mojom-forward.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_news {

TEST(FeedParsingUnitTest, ParsePublisherList) {
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

  std::string feed_json(R"(
    [
      {
        "category": "Sports",
        "publish_time": "2021-09-01 07:04:32",
        "url": "https://www.espn.com/soccer/blog-transfer-talk/story/4465789/live-transfer-deadline-day-will-real-madrid-land-psg-star-mbappe",
        "title": "Transfer Talk: How a busy Deadline Day unfolded",
        "description": "The transfer window is closed and Saul Niguez is on his way to Chelsea, while Antoine Griezmann is set to go back to Atletico Madrid on loan from Barcelona. Check out all the deals from a busy day.",
        "content_type": "article",
        "publisher_id": "111",
        "publisher_name": "ESPN - Football",
        "creative_instance_id": "",
        "url_hash": "7bb5d8b3e2eee9d317f0568dcb094850fdf2862b2ed6d583c62b2245ea507ab8",
        "padded_img": "https://pcdn.brave.com/brave-today/cache/85fb134433369025b46b861a00408e61223678f55620612d980533fa6ce0a815.jpg.pad",
        "score": 13.525910905005045
      },
      {
        "category": "Technology",
        "publish_time": "2021-09-01 07:01:28",
        "url": "https://www.digitaltrends.com/computing/logi-bolt-secure-wireless-connectivity/",
        "title": "Logitech built Bolt to make wireless mice and keyboards work better",
        "description": "Built on top of Bluetooth Low Energy, Logi Bolt is designed to reliably and securely connect wireless mice and keyboard to business PCs.",
        "content_type": "article",
        "publisher_id": "222",
        "publisher_name": "Digital Trends",
        "creative_instance_id": "",
        "url_hash": "523b9f2091474c2a082c06ec17965f8c2392f871917407228bbeb51d8a55d6be",
        "padded_img": "https://pcdn.brave.com/brave-today/cache/052e832456e00a3cee51c68eee206fe71c32cba35d5e53dee2777dd132e01364.jpg.pad",
        "score": 13.91160989810695
      },
      {
        "category": "Top News",
        "publish_time": "2021-09-01 07:00:58",
        "url": "https://foreignpolicy.com/2021/09/01/africa-youth-protests-senegal-sudan-ghana-eswatini/",
        "title": "Africa\u2019s Disappointed Demographic",
        "description": "Young people across the continent have been hit hard by the pandemic, lockdowns, and economic stagnation\u2014but their protests have largely been ignored by elderly elites.",
        "content_type": "article",
        "publisher_id": "333",
        "publisher_name": "Foreign Policy",
        "creative_instance_id": "",
        "url_hash": "9aaa370ed4c2888bc6603404dcc44ed1125d3347101873798d2ec8a0a9c424b1",
        "padded_img": "https://pcdn.brave.com/brave-today/cache/4f7ab8aef2ffb518bc4226d2c50487b6b9bde5f781579288b5b3dde92847db7a.jpg.pad",
        "score": 13.96799592432192
      }
    ]
  )");
  mojom::Feed feed;
  ASSERT_TRUE(ParseFeed(feed_json, &publisher_list, &feed));

  // ASSERT_EQ(token_list[0]->name, "Crypto Kitties");
  // ASSERT_EQ(token_list[0]->contract_address,
  //           "0x06012c8cf97BEaD5deAe237070F9587f8E7A266d");
  // ASSERT_FALSE(token_list[0]->is_erc20);
  // ASSERT_TRUE(token_list[0]->is_erc721);
  // ASSERT_EQ(token_list[0]->symbol, "CK");
  // ASSERT_EQ(token_list[0]->decimals, 0);

  // ASSERT_EQ(token_list[1]->name, "Basic Attention Token");
  // ASSERT_EQ(token_list[1]->contract_address,
  //           "0x0D8775F648430679A709E98d2b0Cb6250d2887EF");
  // ASSERT_TRUE(token_list[1]->is_erc20);
  // ASSERT_FALSE(token_list[1]->is_erc721);
  // ASSERT_EQ(token_list[1]->symbol, "BAT");
  // ASSERT_EQ(token_list[1]->decimals, 18);

  // ASSERT_EQ(token_list[2]->name, "Uniswap");
  // ASSERT_EQ(token_list[2]->contract_address,
  //           "0x1f9840a85d5aF5bf1D1762F925BDADdC4201F984");
  // ASSERT_TRUE(token_list[2]->is_erc20);
  // ASSERT_FALSE(token_list[2]->is_erc721);
  // ASSERT_EQ(token_list[2]->symbol, "UNI");
  // ASSERT_EQ(token_list[2]->decimals, 18);

  // token_list.clear();
  // json = R"({})";
  // ASSERT_TRUE(ParseTokenList(json, &token_list));
  // ASSERT_TRUE(token_list.empty());
  // json = R"({"0x0D8775F648430679A709E98d2b0Cb6250d2887EF": 3})";
  // ASSERT_FALSE(ParseTokenList(json, &token_list));
  // json = R"({"0x0D8775F648430679A709E98d2b0Cb6250d2887EF": {}})";
  // ASSERT_EQ(token_list.size(), 0UL);
  // ASSERT_TRUE(ParseTokenList(json, &token_list));
  // json = "3";
  // ASSERT_FALSE(ParseTokenList(json, &token_list));
  // json = "[3]";
  // ASSERT_FALSE(ParseTokenList(json, &token_list));
  // json = "";
  // ASSERT_FALSE(ParseTokenList(json, &token_list));
}

}  // namespace brave_news
