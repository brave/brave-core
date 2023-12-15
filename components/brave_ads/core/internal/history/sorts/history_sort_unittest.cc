/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/history/sorts/history_sort_factory.h"

#include "base/time/time.h"
#include "brave/components/brave_ads/core/public/history/history_item_info.h"
#include "brave/components/brave_ads/core/public/history/history_sort_types.h"
#include "testing/gmock/include/gmock/gmock.h"  // IWYU pragma: keep
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

HistoryItemList GetUnsortedHistory() {
  HistoryItemList history;

  HistoryItemInfo history_item;
  history_item.created_at = base::Time::FromSecondsSinceUnixEpoch(222222222);
  history.push_back(history_item);
  history_item.created_at = base::Time::FromSecondsSinceUnixEpoch(333333333);
  history.push_back(history_item);
  history_item.created_at = base::Time::FromSecondsSinceUnixEpoch(111111111);
  history.push_back(history_item);
  history_item.created_at = base::Time::FromSecondsSinceUnixEpoch(555555555);
  history.push_back(history_item);
  history_item.created_at = base::Time::FromSecondsSinceUnixEpoch(444444444);
  history.push_back(history_item);

  return history;
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

  HistoryItemList history = GetUnsortedHistory();

  // Act
  sort->Apply(history);

  // Assert
  HistoryItemList expected_history;
  HistoryItemInfo history_item;
  history_item.created_at = base::Time::FromSecondsSinceUnixEpoch(555555555);
  expected_history.push_back(history_item);
  history_item.created_at = base::Time::FromSecondsSinceUnixEpoch(444444444);
  expected_history.push_back(history_item);
  history_item.created_at = base::Time::FromSecondsSinceUnixEpoch(333333333);
  expected_history.push_back(history_item);
  history_item.created_at = base::Time::FromSecondsSinceUnixEpoch(222222222);
  expected_history.push_back(history_item);
  history_item.created_at = base::Time::FromSecondsSinceUnixEpoch(111111111);
  expected_history.push_back(history_item);
  EXPECT_THAT(expected_history, ::testing::ElementsAreArray(history));
}

TEST(BraveAdsHistorySortTest, DescendingSortOrderForEmptyHistory) {
  // Arrange
  const auto sort =
      HistorySortFactory::Build(HistorySortType::kDescendingOrder);

  HistoryItemList history;

  // Act
  sort->Apply(history);

  // Assert
  EXPECT_TRUE(history.empty());
}

TEST(BraveAdsHistorySortTest, AscendingSortOrder) {
  // Arrange
  const auto sort = HistorySortFactory::Build(HistorySortType::kAscendingOrder);

  HistoryItemList history = GetUnsortedHistory();

  // Act
  sort->Apply(history);

  // Assert
  HistoryItemList expected_history;
  HistoryItemInfo history_item;
  history_item.created_at = base::Time::FromSecondsSinceUnixEpoch(111111111);
  expected_history.push_back(history_item);
  history_item.created_at = base::Time::FromSecondsSinceUnixEpoch(222222222);
  expected_history.push_back(history_item);
  history_item.created_at = base::Time::FromSecondsSinceUnixEpoch(333333333);
  expected_history.push_back(history_item);
  history_item.created_at = base::Time::FromSecondsSinceUnixEpoch(444444444);
  expected_history.push_back(history_item);
  history_item.created_at = base::Time::FromSecondsSinceUnixEpoch(555555555);
  expected_history.push_back(history_item);
  EXPECT_THAT(expected_history, ::testing::ElementsAreArray(history));
}

TEST(BraveAdsHistorySortTest, AscendingSortOrderForEmptyHistory) {
  // Arrange
  const auto sort = HistorySortFactory::Build(HistorySortType::kAscendingOrder);

  HistoryItemList history;

  // Act
  sort->Apply(history);

  // Assert
  EXPECT_TRUE(history.empty());
}

}  // namespace brave_ads
