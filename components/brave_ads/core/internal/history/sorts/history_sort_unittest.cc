/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/history/sorts/history_sort_factory.h"

#include "base/ranges/algorithm.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/public/history/history_item_info.h"
#include "brave/components/brave_ads/core/public/history/history_sort_types.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

HistoryItemList GetUnsortedHistory() {
  HistoryItemList history;

  HistoryItemInfo history_item;
  history_item.created_at = base::Time::FromDoubleT(222222222);
  history.push_back(history_item);
  history_item.created_at = base::Time::FromDoubleT(333333333);
  history.push_back(history_item);
  history_item.created_at = base::Time::FromDoubleT(111111111);
  history.push_back(history_item);
  history_item.created_at = base::Time::FromDoubleT(555555555);
  history.push_back(history_item);
  history_item.created_at = base::Time::FromDoubleT(444444444);
  history.push_back(history_item);

  return history;
}

}  // namespace

TEST(BraveAdsHistorySortTest, NoSortOrder) {
  // Arrange

  // Act
  const auto sort = HistorySortFactory::Build(HistorySortType::kNone);

  // Assert
  EXPECT_EQ(nullptr, sort);
}

TEST(BraveAdsHistorySortTest, DescendingSortOrder) {
  // Arrange
  const auto sort =
      HistorySortFactory::Build(HistorySortType::kDescendingOrder);

  HistoryItemList history = GetUnsortedHistory();

  // Act
  history = sort->Apply(history);

  // Assert
  HistoryItemList expected_history;
  HistoryItemInfo history_item;
  history_item.created_at = base::Time::FromDoubleT(555555555);
  expected_history.push_back(history_item);
  history_item.created_at = base::Time::FromDoubleT(444444444);
  expected_history.push_back(history_item);
  history_item.created_at = base::Time::FromDoubleT(333333333);
  expected_history.push_back(history_item);
  history_item.created_at = base::Time::FromDoubleT(222222222);
  expected_history.push_back(history_item);
  history_item.created_at = base::Time::FromDoubleT(111111111);
  expected_history.push_back(history_item);

  EXPECT_TRUE(base::ranges::equal(expected_history, history));
}

TEST(BraveAdsHistorySortTest, DescendingSortOrderForEmptyHistory) {
  // Arrange
  const auto sort =
      HistorySortFactory::Build(HistorySortType::kDescendingOrder);

  HistoryItemList history;

  // Act
  history = sort->Apply(history);

  // Assert
  EXPECT_TRUE(history.empty());
}

TEST(BraveAdsHistorySortTest, AscendingSortOrder) {
  // Arrange
  const auto sort = HistorySortFactory::Build(HistorySortType::kAscendingOrder);

  HistoryItemList expected_history;
  HistoryItemInfo history_item;
  history_item.created_at = base::Time::FromDoubleT(111111111);
  expected_history.push_back(history_item);
  history_item.created_at = base::Time::FromDoubleT(222222222);
  expected_history.push_back(history_item);
  history_item.created_at = base::Time::FromDoubleT(333333333);
  expected_history.push_back(history_item);
  history_item.created_at = base::Time::FromDoubleT(444444444);
  expected_history.push_back(history_item);
  history_item.created_at = base::Time::FromDoubleT(555555555);
  expected_history.push_back(history_item);

  HistoryItemList history = GetUnsortedHistory();

  // Act
  history = sort->Apply(history);

  // Assert
  EXPECT_TRUE(base::ranges::equal(expected_history, history));
}

TEST(BraveAdsHistorySortTest, AscendingSortOrderForEmptyHistory) {
  // Arrange
  const auto sort = HistorySortFactory::Build(HistorySortType::kAscendingOrder);

  HistoryItemList history;

  // Act
  history = sort->Apply(history);

  // Assert
  EXPECT_TRUE(history.empty());
}

}  // namespace brave_ads
