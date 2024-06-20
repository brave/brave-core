/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/history/sorts/history_sort_factory.h"
#include "brave/components/brave_ads/core/public/history/history_item_info.h"
#include "brave/components/brave_ads/core/public/history/history_sort_types.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

HistoryItemList GetUnsortedHistory() {
  HistoryItemList history_items;

  HistoryItemInfo history_item;
  history_item.created_at = base::Time::FromSecondsSinceUnixEpoch(222222222);
  history_items.push_back(history_item);
  history_item.created_at = base::Time::FromSecondsSinceUnixEpoch(333333333);
  history_items.push_back(history_item);
  history_item.created_at = base::Time::FromSecondsSinceUnixEpoch(111111111);
  history_items.push_back(history_item);
  history_item.created_at = base::Time::FromSecondsSinceUnixEpoch(555555555);
  history_items.push_back(history_item);
  history_item.created_at = base::Time::FromSecondsSinceUnixEpoch(444444444);
  history_items.push_back(history_item);

  return history_items;
}

}  // namespace

TEST(BraveAdsHistorySortTest, NoSortOrder) {
  // Act & Assert
  EXPECT_EQ(nullptr, HistorySortFactory::Build(HistorySortType::kNone));
}

TEST(BraveAdsHistorySortTest, DescendingSortOrder) {
  // Arrange
  const auto sort =
      HistorySortFactory::Build(HistorySortType::kDescendingOrder);

  HistoryItemList history_items = GetUnsortedHistory();

  // Act
  sort->Apply(history_items);

  // Assert
  HistoryItemList expected_history_items;
  HistoryItemInfo expected_history_item;
  expected_history_item.created_at =
      base::Time::FromSecondsSinceUnixEpoch(555555555);
  expected_history_items.push_back(expected_history_item);
  expected_history_item.created_at =
      base::Time::FromSecondsSinceUnixEpoch(444444444);
  expected_history_items.push_back(expected_history_item);
  expected_history_item.created_at =
      base::Time::FromSecondsSinceUnixEpoch(333333333);
  expected_history_items.push_back(expected_history_item);
  expected_history_item.created_at =
      base::Time::FromSecondsSinceUnixEpoch(222222222);
  expected_history_items.push_back(expected_history_item);
  expected_history_item.created_at =
      base::Time::FromSecondsSinceUnixEpoch(111111111);
  expected_history_items.push_back(expected_history_item);
  EXPECT_THAT(expected_history_items,
              ::testing::ElementsAreArray(history_items));
}

TEST(BraveAdsHistorySortTest, DescendingSortOrderForEmptyHistory) {
  // Arrange
  const auto sort =
      HistorySortFactory::Build(HistorySortType::kDescendingOrder);

  HistoryItemList history_items;

  // Act
  sort->Apply(history_items);

  // Assert
  EXPECT_THAT(history_items, ::testing::IsEmpty());
}

TEST(BraveAdsHistorySortTest, AscendingSortOrder) {
  // Arrange
  const auto sort = HistorySortFactory::Build(HistorySortType::kAscendingOrder);

  HistoryItemList history_items = GetUnsortedHistory();

  // Act
  sort->Apply(history_items);

  // Assert
  HistoryItemList expected_history_items;
  HistoryItemInfo expected_history_item;
  expected_history_item.created_at =
      base::Time::FromSecondsSinceUnixEpoch(111111111);
  expected_history_items.push_back(expected_history_item);
  expected_history_item.created_at =
      base::Time::FromSecondsSinceUnixEpoch(222222222);
  expected_history_items.push_back(expected_history_item);
  expected_history_item.created_at =
      base::Time::FromSecondsSinceUnixEpoch(333333333);
  expected_history_items.push_back(expected_history_item);
  expected_history_item.created_at =
      base::Time::FromSecondsSinceUnixEpoch(444444444);
  expected_history_items.push_back(expected_history_item);
  expected_history_item.created_at =
      base::Time::FromSecondsSinceUnixEpoch(555555555);
  expected_history_items.push_back(expected_history_item);
  EXPECT_THAT(expected_history_items,
              ::testing::ElementsAreArray(history_items));
}

TEST(BraveAdsHistorySortTest, AscendingSortOrderForEmptyHistory) {
  // Arrange
  const auto sort = HistorySortFactory::Build(HistorySortType::kAscendingOrder);

  HistoryItemList history_items;

  // Act
  sort->Apply(history_items);

  // Assert
  EXPECT_THAT(history_items, ::testing::IsEmpty());
}

}  // namespace brave_ads
