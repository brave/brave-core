/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/history/sorts/history_sort_factory.h"

#include "base/containers/circular_deque.h"
#include "bat/ads/history_info.h"
#include "bat/ads/history_item_info.h"
#include "bat/ads/history_sort_types.h"
#include "bat/ads/internal/base/container_util.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {

base::circular_deque<HistoryItemInfo> GetUnsortedHistory() {
  base::circular_deque<HistoryItemInfo> history;

  HistoryItemInfo history_item;
  history_item.time = base::Time::FromDoubleT(22222222222);
  history.push_back(history_item);
  history_item.time = base::Time::FromDoubleT(33333333333);
  history.push_back(history_item);
  history_item.time = base::Time::FromDoubleT(11111111111);
  history.push_back(history_item);
  history_item.time = base::Time::FromDoubleT(55555555555);
  history.push_back(history_item);
  history_item.time = base::Time::FromDoubleT(44444444444);
  history.push_back(history_item);

  return history;
}

}  // namespace

TEST(BatAdsHistorySortTest, NoSortOrder) {
  // Arrange

  // Act
  const auto sort = HistorySortFactory::Build(HistorySortType::kNone);

  // Assert
  EXPECT_EQ(nullptr, sort);
}

TEST(BatAdsHistorySortTest, DescendingSortOrder) {
  // Arrange
  const auto sort =
      HistorySortFactory::Build(HistorySortType::kDescendingOrder);

  base::circular_deque<HistoryItemInfo> history = GetUnsortedHistory();

  // Act
  history = sort->Apply(history);

  // Assert
  base::circular_deque<HistoryItemInfo> expected_history;
  HistoryItemInfo history_item;
  history_item.time = base::Time::FromDoubleT(55555555555);
  expected_history.push_back(history_item);
  history_item.time = base::Time::FromDoubleT(44444444444);
  expected_history.push_back(history_item);
  history_item.time = base::Time::FromDoubleT(33333333333);
  expected_history.push_back(history_item);
  history_item.time = base::Time::FromDoubleT(22222222222);
  expected_history.push_back(history_item);
  history_item.time = base::Time::FromDoubleT(11111111111);
  expected_history.push_back(history_item);

  EXPECT_TRUE(IsEqualContainers(expected_history, history));
}

TEST(BatAdsHistorySortTest, DescendingSortOrderForEmptyHistory) {
  // Arrange
  const auto sort =
      HistorySortFactory::Build(HistorySortType::kDescendingOrder);

  base::circular_deque<HistoryItemInfo> history = {};

  // Act
  history = sort->Apply(history);

  // Assert
  const base::circular_deque<HistoryItemInfo> expected_history = {};

  EXPECT_TRUE(IsEqualContainers(expected_history, history));
}

TEST(BatAdsHistorySortTest, AscendingSortOrder) {
  // Arrange
  const auto sort = HistorySortFactory::Build(HistorySortType::kAscendingOrder);

  base::circular_deque<HistoryItemInfo> expected_history;
  HistoryItemInfo history_item;
  history_item.time = base::Time::FromDoubleT(11111111111);
  expected_history.push_back(history_item);
  history_item.time = base::Time::FromDoubleT(22222222222);
  expected_history.push_back(history_item);
  history_item.time = base::Time::FromDoubleT(33333333333);
  expected_history.push_back(history_item);
  history_item.time = base::Time::FromDoubleT(44444444444);
  expected_history.push_back(history_item);
  history_item.time = base::Time::FromDoubleT(55555555555);
  expected_history.push_back(history_item);

  base::circular_deque<HistoryItemInfo> history = GetUnsortedHistory();

  // Act
  history = sort->Apply(history);

  // Assert
  EXPECT_TRUE(IsEqualContainers(expected_history, history));
}

TEST(BatAdsHistorySortTest, AscendingSortOrderForEmptyHistory) {
  // Arrange
  const auto sort = HistorySortFactory::Build(HistorySortType::kAscendingOrder);

  base::circular_deque<HistoryItemInfo> expected_history;
  base::circular_deque<HistoryItemInfo> history;

  // Act
  history = sort->Apply(history);

  // Assert
  EXPECT_TRUE(IsEqualContainers(expected_history, history));
}

}  // namespace ads
