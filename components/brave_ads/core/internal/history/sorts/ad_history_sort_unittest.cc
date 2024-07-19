/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/history/sorts/ad_history_sort_factory.h"
#include "brave/components/brave_ads/core/public/history/ad_history_item_info.h"
#include "brave/components/brave_ads/core/public/history/ad_history_sort_types.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

AdHistoryList BuildUnsortedAdHistory() {
  AdHistoryList ad_history;

  AdHistoryItemInfo ad_history_item;
  ad_history_item.created_at = base::Time::FromSecondsSinceUnixEpoch(222222222);
  ad_history.push_back(ad_history_item);
  ad_history_item.created_at = base::Time::FromSecondsSinceUnixEpoch(333333333);
  ad_history.push_back(ad_history_item);
  ad_history_item.created_at = base::Time::FromSecondsSinceUnixEpoch(111111111);
  ad_history.push_back(ad_history_item);
  ad_history_item.created_at = base::Time::FromSecondsSinceUnixEpoch(555555555);
  ad_history.push_back(ad_history_item);
  ad_history_item.created_at = base::Time::FromSecondsSinceUnixEpoch(444444444);
  ad_history.push_back(ad_history_item);

  return ad_history;
}

}  // namespace

TEST(BraveAdsAdHistorySortTest, NoSortOrder) {
  // Act & Assert
  EXPECT_EQ(nullptr, AdHistorySortFactory::Build(AdHistorySortType::kNone));
}

TEST(BraveAdsAdHistorySortTest, DescendingSortOrder) {
  // Arrange
  const auto sort =
      AdHistorySortFactory::Build(AdHistorySortType::kDescendingOrder);

  AdHistoryList ad_history = BuildUnsortedAdHistory();

  // Act
  sort->Apply(ad_history);

  // Assert
  AdHistoryList expected_ad_history;
  AdHistoryItemInfo expected_ad_history_item;
  expected_ad_history_item.created_at =
      base::Time::FromSecondsSinceUnixEpoch(555555555);
  expected_ad_history.push_back(expected_ad_history_item);
  expected_ad_history_item.created_at =
      base::Time::FromSecondsSinceUnixEpoch(444444444);
  expected_ad_history.push_back(expected_ad_history_item);
  expected_ad_history_item.created_at =
      base::Time::FromSecondsSinceUnixEpoch(333333333);
  expected_ad_history.push_back(expected_ad_history_item);
  expected_ad_history_item.created_at =
      base::Time::FromSecondsSinceUnixEpoch(222222222);
  expected_ad_history.push_back(expected_ad_history_item);
  expected_ad_history_item.created_at =
      base::Time::FromSecondsSinceUnixEpoch(111111111);
  expected_ad_history.push_back(expected_ad_history_item);
  EXPECT_THAT(expected_ad_history, ::testing::ElementsAreArray(ad_history));
}

TEST(BraveAdsAdHistorySortTest, DescendingSortOrderForEmptyHistory) {
  // Arrange
  const auto sort =
      AdHistorySortFactory::Build(AdHistorySortType::kDescendingOrder);

  AdHistoryList ad_history;

  // Act
  sort->Apply(ad_history);

  // Assert
  EXPECT_THAT(ad_history, ::testing::IsEmpty());
}

TEST(BraveAdsAdHistorySortTest, AscendingSortOrder) {
  // Arrange
  const auto sort =
      AdHistorySortFactory::Build(AdHistorySortType::kAscendingOrder);

  AdHistoryList ad_history = BuildUnsortedAdHistory();

  // Act
  sort->Apply(ad_history);

  // Assert
  AdHistoryList expected_ad_history;
  AdHistoryItemInfo expected_ad_history_item;
  expected_ad_history_item.created_at =
      base::Time::FromSecondsSinceUnixEpoch(111111111);
  expected_ad_history.push_back(expected_ad_history_item);
  expected_ad_history_item.created_at =
      base::Time::FromSecondsSinceUnixEpoch(222222222);
  expected_ad_history.push_back(expected_ad_history_item);
  expected_ad_history_item.created_at =
      base::Time::FromSecondsSinceUnixEpoch(333333333);
  expected_ad_history.push_back(expected_ad_history_item);
  expected_ad_history_item.created_at =
      base::Time::FromSecondsSinceUnixEpoch(444444444);
  expected_ad_history.push_back(expected_ad_history_item);
  expected_ad_history_item.created_at =
      base::Time::FromSecondsSinceUnixEpoch(555555555);
  expected_ad_history.push_back(expected_ad_history_item);
  EXPECT_THAT(expected_ad_history, ::testing::ElementsAreArray(ad_history));
}

TEST(BraveAdsAdHistorySortTest, AscendingSortOrderForEmptyHistory) {
  // Arrange
  const auto sort =
      AdHistorySortFactory::Build(AdHistorySortType::kAscendingOrder);

  AdHistoryList ad_history;

  // Act
  sort->Apply(ad_history);

  // Assert
  EXPECT_THAT(ad_history, ::testing::IsEmpty());
}

}  // namespace brave_ads
