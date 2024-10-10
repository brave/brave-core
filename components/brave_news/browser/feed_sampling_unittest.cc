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

namespace brave_news {

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

  infos.push_back({mojom::FeedItemMetadata::New(), ArticleMetadata()});

  EXPECT_EQ(0u, PickFirstIndex(infos).value());

  infos.push_back({mojom::FeedItemMetadata::New(), ArticleMetadata()});
  infos.push_back({mojom::FeedItemMetadata::New(), ArticleMetadata()});
  infos.push_back({mojom::FeedItemMetadata::New(), ArticleMetadata()});

  EXPECT_EQ(0u, PickFirstIndex(infos).value());
}

TEST(BraveNewsFeedSampling, PickRouletteDoesntBreakOnEmptyList) {
  ArticleInfos infos;

  EXPECT_EQ(std::nullopt, PickRoulette(infos));
}

TEST(BraveNewsFeedSampling, PickRouletteWithWeighting) {
  ArticleInfos infos;
  infos.push_back({mojom::FeedItemMetadata::New(), ArticleMetadata()});
  infos.push_back({mojom::FeedItemMetadata::New(), ArticleMetadata()});
  infos.push_back({mojom::FeedItemMetadata::New(), ArticleMetadata()});

  // No positively weighted items, so we shouldn't pick anything.
  EXPECT_EQ(std::nullopt,
            PickRouletteWithWeighting(
                infos, base::BindRepeating(
                           [](const mojom::FeedItemMetadataPtr& item,
                              const ArticleMetadata& meta) { return 0.0; })));
  auto& first = std::get<0>(infos.at(0));
  auto& second = std::get<0>(infos.at(1));
  auto& third = std::get<0>(infos.at(2));
  auto make_picker_for =
      [](const mojom::FeedItemMetadataPtr& target) -> GetWeighting {
    return base::BindRepeating(
        [](mojom::FeedItemMetadata* target,
           const mojom::FeedItemMetadataPtr& item,
           const ArticleMetadata& meta) {
          return target == item.get() ? 100.0 : 0.0;
        },
        target.get());
  };

  EXPECT_EQ(0, PickRouletteWithWeighting(infos, make_picker_for(first)));
  EXPECT_EQ(1, PickRouletteWithWeighting(infos, make_picker_for(second)));
  EXPECT_EQ(2, PickRouletteWithWeighting(infos, make_picker_for(third)));
}

}  // namespace brave_news
