/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/history/filters/date_range_history_filter.h"

#include "base/containers/circular_deque.h"
#include "base/ranges/algorithm.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/public/history/history_item_info.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

HistoryItemList GetHistory() {
  HistoryItemList history;

  HistoryItemInfo history_item;
  history_item.created_at = base::Time::FromDoubleT(333333333);
  history.push_back(history_item);
  history_item.created_at = base::Time::FromDoubleT(444444444);
  history.push_back(history_item);
  history_item.created_at = base::Time::FromDoubleT(222222222);
  history.push_back(history_item);
  history_item.created_at = base::Time::FromDoubleT(666666666);
  history.push_back(history_item);
  history_item.created_at = base::Time::FromDoubleT(555555555);
  history.push_back(history_item);

  return history;
}

}  // namespace

TEST(BraveAdsDateRangeHistoryFilterTest,
     FilterHistoryFromTimestamp444444444ToDistantFuture) {
  // Arrange
  HistoryItemList history = GetHistory();

  const base::Time from_time = base::Time::FromDoubleT(444444444);
  const base::Time to_time = DistantFuture();

  // Act
  const DateRangeHistoryFilter filter(from_time, to_time);
  history = filter.Apply(history);

  // Assert
  HistoryItemList expected_history;

  HistoryItemInfo history_item;
  history_item.created_at = base::Time::FromDoubleT(444444444);
  expected_history.push_back(history_item);
  history_item.created_at = base::Time::FromDoubleT(666666666);
  expected_history.push_back(history_item);
  history_item.created_at = base::Time::FromDoubleT(555555555);
  expected_history.push_back(history_item);

  EXPECT_TRUE(base::ranges::equal(expected_history, history));
}

TEST(BraveAdsDateRangeHistoryFilterTest,
     FilterHistoryFromTimestamp777777777ToDistantFuture) {
  // Arrange
  HistoryItemList history = GetHistory();

  const base::Time from_time = base::Time::FromDoubleT(777777777);
  const base::Time to_time = DistantFuture();

  // Act
  const DateRangeHistoryFilter filter(from_time, to_time);
  history = filter.Apply(history);

  // Assert
  EXPECT_TRUE(history.empty());
}

TEST(BraveAdsDateRangeHistoryFilterTest,
     FilterHistoryFromDistantPastToTimestamp444444444) {
  // Arrange
  HistoryItemList history = GetHistory();

  const base::Time from_time = DistantPast();
  const base::Time to_time = base::Time::FromDoubleT(444444444);

  // Act
  const DateRangeHistoryFilter filter(from_time, to_time);
  history = filter.Apply(history);

  // Assert
  HistoryItemList expected_history;
  HistoryItemInfo history_item;
  history_item.created_at = base::Time::FromDoubleT(333333333);
  expected_history.push_back(history_item);
  history_item.created_at = base::Time::FromDoubleT(444444444);
  expected_history.push_back(history_item);
  history_item.created_at = base::Time::FromDoubleT(222222222);
  expected_history.push_back(history_item);

  EXPECT_TRUE(base::ranges::equal(expected_history, history));
}

TEST(BraveAdsDateRangeHistoryFilterTest,
     FilterHistoryFromDistancePastToTimestamp111111111) {
  // Arrange
  HistoryItemList history = GetHistory();

  const base::Time from_time = DistantPast();
  const base::Time to_time = base::Time::FromDoubleT(111111111);

  // Act
  const DateRangeHistoryFilter filter(from_time, to_time);
  history = filter.Apply(history);

  // Assert
  EXPECT_TRUE(history.empty());
}

TEST(BraveAdsDateRangeHistoryFilterTest,
     FilterHistoryFromDistantPastToDistantFuture) {
  // Arrange
  HistoryItemList history = GetHistory();

  const base::Time from_time = DistantPast();
  const base::Time to_time = DistantFuture();

  // Act
  const DateRangeHistoryFilter filter(from_time, to_time);
  history = filter.Apply(history);

  // Assert
  HistoryItemList expected_history;
  HistoryItemInfo history_item;
  history_item.created_at = base::Time::FromDoubleT(333333333);
  expected_history.push_back(history_item);
  history_item.created_at = base::Time::FromDoubleT(444444444);
  expected_history.push_back(history_item);
  history_item.created_at = base::Time::FromDoubleT(222222222);
  expected_history.push_back(history_item);
  history_item.created_at = base::Time::FromDoubleT(666666666);
  expected_history.push_back(history_item);
  history_item.created_at = base::Time::FromDoubleT(555555555);
  expected_history.push_back(history_item);

  EXPECT_TRUE(base::ranges::equal(expected_history, history));
}

TEST(BraveAdsDateRangeHistoryFilterTest,
     FilterHistoryFromDistantFutureToDistantPast) {
  // Arrange
  HistoryItemList history = GetHistory();

  const base::Time from_time = DistantFuture();
  const base::Time to_time = DistantPast();

  // Act
  const DateRangeHistoryFilter filter(from_time, to_time);
  history = filter.Apply(history);

  // Assert
  EXPECT_TRUE(history.empty());
}

TEST(BraveAdsDateRangeHistoryFilterTest, FilterEmptyHistory) {
  // Arrange
  HistoryItemList history;

  const base::Time from_time = base::Time::FromDoubleT(444444444);
  const base::Time to_time = DistantFuture();

  // Act
  const DateRangeHistoryFilter filter(from_time, to_time);
  history = filter.Apply(history);

  // Assert
  EXPECT_TRUE(history.empty());
}

}  // namespace brave_ads
