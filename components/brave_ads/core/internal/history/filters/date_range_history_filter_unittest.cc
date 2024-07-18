/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/history/filters/date_range_history_filter.h"

#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "brave/components/brave_ads/core/public/history/history_item_info.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

HistoryItemList GetHistory() {
  HistoryItemList history_items;

  HistoryItemInfo history_item;
  history_item.created_at = base::Time::FromSecondsSinceUnixEpoch(333333333);
  history_items.push_back(history_item);
  history_item.created_at = base::Time::FromSecondsSinceUnixEpoch(444444444);
  history_items.push_back(history_item);
  history_item.created_at = base::Time::FromSecondsSinceUnixEpoch(222222222);
  history_items.push_back(history_item);
  history_item.created_at = base::Time::FromSecondsSinceUnixEpoch(666666666);
  history_items.push_back(history_item);
  history_item.created_at = base::Time::FromSecondsSinceUnixEpoch(555555555);
  history_items.push_back(history_item);

  return history_items;
}

}  // namespace

TEST(BraveAdsDateRangeHistoryFilterTest,
     FilterHistoryFromTimestamp444444444ToDistantFuture) {
  // Arrange
  HistoryItemList history_items = GetHistory();

  const base::Time from_time = base::Time::FromSecondsSinceUnixEpoch(444444444);
  const base::Time to_time = test::DistantFuture();

  const DateRangeHistoryFilter filter(from_time, to_time);

  // Act
  filter.Apply(history_items);

  // Assert
  HistoryItemList expected_history_items;
  HistoryItemInfo expected_history_item;
  expected_history_item.created_at =
      base::Time::FromSecondsSinceUnixEpoch(444444444);
  expected_history_items.push_back(expected_history_item);
  expected_history_item.created_at =
      base::Time::FromSecondsSinceUnixEpoch(666666666);
  expected_history_items.push_back(expected_history_item);
  expected_history_item.created_at =
      base::Time::FromSecondsSinceUnixEpoch(555555555);
  expected_history_items.push_back(expected_history_item);
  EXPECT_THAT(expected_history_items,
              ::testing::ElementsAreArray(history_items));
}

TEST(BraveAdsDateRangeHistoryFilterTest,
     FilterHistoryFromTimestamp777777777ToDistantFuture) {
  // Arrange
  HistoryItemList history_items = GetHistory();

  const base::Time from_time = base::Time::FromSecondsSinceUnixEpoch(777777777);
  const base::Time to_time = test::DistantFuture();

  const DateRangeHistoryFilter filter(from_time, to_time);

  // Act
  filter.Apply(history_items);

  // Assert
  EXPECT_THAT(history_items, ::testing::IsEmpty());
}

TEST(BraveAdsDateRangeHistoryFilterTest,
     FilterHistoryFromDistantPastToTimestamp444444444) {
  // Arrange
  HistoryItemList history_items = GetHistory();

  const base::Time from_time = test::DistantPast();
  const base::Time to_time = base::Time::FromSecondsSinceUnixEpoch(444444444);

  const DateRangeHistoryFilter filter(from_time, to_time);

  // Act
  filter.Apply(history_items);

  // Assert
  HistoryItemList expected_history_items;
  HistoryItemInfo expected_history_item;
  expected_history_item.created_at =
      base::Time::FromSecondsSinceUnixEpoch(333333333);
  expected_history_items.push_back(expected_history_item);
  expected_history_item.created_at =
      base::Time::FromSecondsSinceUnixEpoch(444444444);
  expected_history_items.push_back(expected_history_item);
  expected_history_item.created_at =
      base::Time::FromSecondsSinceUnixEpoch(222222222);
  expected_history_items.push_back(expected_history_item);
  EXPECT_THAT(expected_history_items,
              ::testing::ElementsAreArray(history_items));
}

TEST(BraveAdsDateRangeHistoryFilterTest,
     FilterHistoryFromDistancePastToTimestamp111111111) {
  // Arrange
  HistoryItemList history_items = GetHistory();

  const base::Time from_time = test::DistantPast();
  const base::Time to_time = base::Time::FromSecondsSinceUnixEpoch(111111111);

  const DateRangeHistoryFilter filter(from_time, to_time);

  // Act
  filter.Apply(history_items);

  // Assert
  EXPECT_THAT(history_items, ::testing::IsEmpty());
}

TEST(BraveAdsDateRangeHistoryFilterTest,
     FilterHistoryFromDistantPastToDistantFuture) {
  // Arrange
  HistoryItemList history_items = GetHistory();

  const base::Time from_time = test::DistantPast();
  const base::Time to_time = test::DistantFuture();

  const DateRangeHistoryFilter filter(from_time, to_time);

  // Act
  filter.Apply(history_items);

  // Assert
  HistoryItemList expected_history_items;
  HistoryItemInfo expected_history_item;
  expected_history_item.created_at =
      base::Time::FromSecondsSinceUnixEpoch(333333333);
  expected_history_items.push_back(expected_history_item);
  expected_history_item.created_at =
      base::Time::FromSecondsSinceUnixEpoch(444444444);
  expected_history_items.push_back(expected_history_item);
  expected_history_item.created_at =
      base::Time::FromSecondsSinceUnixEpoch(222222222);
  expected_history_items.push_back(expected_history_item);
  expected_history_item.created_at =
      base::Time::FromSecondsSinceUnixEpoch(666666666);
  expected_history_items.push_back(expected_history_item);
  expected_history_item.created_at =
      base::Time::FromSecondsSinceUnixEpoch(555555555);
  expected_history_items.push_back(expected_history_item);
  EXPECT_THAT(expected_history_items,
              ::testing::ElementsAreArray(history_items));
}

TEST(BraveAdsDateRangeHistoryFilterTest,
     FilterHistoryFromDistantFutureToDistantPast) {
  // Arrange
  HistoryItemList history_items = GetHistory();

  const base::Time from_time = test::DistantFuture();
  const base::Time to_time = test::DistantPast();

  const DateRangeHistoryFilter filter(from_time, to_time);

  // Act
  filter.Apply(history_items);

  // Assert
  EXPECT_THAT(history_items, ::testing::IsEmpty());
}

TEST(BraveAdsDateRangeHistoryFilterTest, FilterEmptyHistory) {
  // Arrange
  HistoryItemList history_items;

  const base::Time from_time = base::Time::FromSecondsSinceUnixEpoch(444444444);
  const base::Time to_time = test::DistantFuture();

  const DateRangeHistoryFilter filter(from_time, to_time);

  // Act
  filter.Apply(history_items);

  // Assert
  EXPECT_THAT(history_items, ::testing::IsEmpty());
}

}  // namespace brave_ads
