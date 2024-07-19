/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/history/filters/ad_history_date_range_filter.h"

#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "brave/components/brave_ads/core/public/history/ad_history_item_info.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

AdHistoryList BuildAdHistory() {
  AdHistoryList ad_history;

  AdHistoryItemInfo ad_history_item;
  ad_history_item.created_at = base::Time::FromSecondsSinceUnixEpoch(333333333);
  ad_history.push_back(ad_history_item);
  ad_history_item.created_at = base::Time::FromSecondsSinceUnixEpoch(444444444);
  ad_history.push_back(ad_history_item);
  ad_history_item.created_at = base::Time::FromSecondsSinceUnixEpoch(222222222);
  ad_history.push_back(ad_history_item);
  ad_history_item.created_at = base::Time::FromSecondsSinceUnixEpoch(666666666);
  ad_history.push_back(ad_history_item);
  ad_history_item.created_at = base::Time::FromSecondsSinceUnixEpoch(555555555);
  ad_history.push_back(ad_history_item);

  return ad_history;
}

}  // namespace

TEST(BraveAdsAdHistoryDateRangeFilterTest,
     FilterHistoryFromTimestamp444444444ToDistantFuture) {
  // Arrange
  AdHistoryList ad_history = BuildAdHistory();

  const base::Time from_time = base::Time::FromSecondsSinceUnixEpoch(444444444);
  const base::Time to_time = test::DistantFuture();

  const AdHistoryDateRangeFilter filter(from_time, to_time);

  // Act
  filter.Apply(ad_history);

  // Assert
  AdHistoryList expected_ad_history;
  AdHistoryItemInfo expected_ad_history_item;
  expected_ad_history_item.created_at =
      base::Time::FromSecondsSinceUnixEpoch(444444444);
  expected_ad_history.push_back(expected_ad_history_item);
  expected_ad_history_item.created_at =
      base::Time::FromSecondsSinceUnixEpoch(666666666);
  expected_ad_history.push_back(expected_ad_history_item);
  expected_ad_history_item.created_at =
      base::Time::FromSecondsSinceUnixEpoch(555555555);
  expected_ad_history.push_back(expected_ad_history_item);
  EXPECT_THAT(expected_ad_history, ::testing::ElementsAreArray(ad_history));
}

TEST(BraveAdsAdHistoryDateRangeFilterTest,
     FilterHistoryFromTimestamp777777777ToDistantFuture) {
  // Arrange
  AdHistoryList ad_history = BuildAdHistory();

  const base::Time from_time = base::Time::FromSecondsSinceUnixEpoch(777777777);
  const base::Time to_time = test::DistantFuture();

  const AdHistoryDateRangeFilter filter(from_time, to_time);

  // Act
  filter.Apply(ad_history);

  // Assert
  EXPECT_THAT(ad_history, ::testing::IsEmpty());
}

TEST(BraveAdsAdHistoryDateRangeFilterTest,
     FilterHistoryFromDistantPastToTimestamp444444444) {
  // Arrange
  AdHistoryList ad_history = BuildAdHistory();

  const base::Time from_time = test::DistantPast();
  const base::Time to_time = base::Time::FromSecondsSinceUnixEpoch(444444444);

  const AdHistoryDateRangeFilter filter(from_time, to_time);

  // Act
  filter.Apply(ad_history);

  // Assert
  AdHistoryList expected_ad_history;
  AdHistoryItemInfo expected_ad_history_item;
  expected_ad_history_item.created_at =
      base::Time::FromSecondsSinceUnixEpoch(333333333);
  expected_ad_history.push_back(expected_ad_history_item);
  expected_ad_history_item.created_at =
      base::Time::FromSecondsSinceUnixEpoch(444444444);
  expected_ad_history.push_back(expected_ad_history_item);
  expected_ad_history_item.created_at =
      base::Time::FromSecondsSinceUnixEpoch(222222222);
  expected_ad_history.push_back(expected_ad_history_item);
  EXPECT_THAT(expected_ad_history, ::testing::ElementsAreArray(ad_history));
}

TEST(BraveAdsAdHistoryDateRangeFilterTest,
     FilterHistoryFromDistancePastToTimestamp111111111) {
  // Arrange
  AdHistoryList ad_history = BuildAdHistory();

  const base::Time from_time = test::DistantPast();
  const base::Time to_time = base::Time::FromSecondsSinceUnixEpoch(111111111);

  const AdHistoryDateRangeFilter filter(from_time, to_time);

  // Act
  filter.Apply(ad_history);

  // Assert
  EXPECT_THAT(ad_history, ::testing::IsEmpty());
}

TEST(BraveAdsAdHistoryDateRangeFilterTest,
     FilterHistoryFromDistantPastToDistantFuture) {
  // Arrange
  AdHistoryList ad_history = BuildAdHistory();

  const base::Time from_time = test::DistantPast();
  const base::Time to_time = test::DistantFuture();

  const AdHistoryDateRangeFilter filter(from_time, to_time);

  // Act
  filter.Apply(ad_history);

  // Assert
  AdHistoryList expected_ad_history;
  AdHistoryItemInfo expected_ad_history_item;
  expected_ad_history_item.created_at =
      base::Time::FromSecondsSinceUnixEpoch(333333333);
  expected_ad_history.push_back(expected_ad_history_item);
  expected_ad_history_item.created_at =
      base::Time::FromSecondsSinceUnixEpoch(444444444);
  expected_ad_history.push_back(expected_ad_history_item);
  expected_ad_history_item.created_at =
      base::Time::FromSecondsSinceUnixEpoch(222222222);
  expected_ad_history.push_back(expected_ad_history_item);
  expected_ad_history_item.created_at =
      base::Time::FromSecondsSinceUnixEpoch(666666666);
  expected_ad_history.push_back(expected_ad_history_item);
  expected_ad_history_item.created_at =
      base::Time::FromSecondsSinceUnixEpoch(555555555);
  expected_ad_history.push_back(expected_ad_history_item);
  EXPECT_THAT(expected_ad_history, ::testing::ElementsAreArray(ad_history));
}

TEST(BraveAdsAdHistoryDateRangeFilterTest,
     FilterHistoryFromDistantFutureToDistantPast) {
  // Arrange
  AdHistoryList ad_history = BuildAdHistory();

  const base::Time from_time = test::DistantFuture();
  const base::Time to_time = test::DistantPast();

  const AdHistoryDateRangeFilter filter(from_time, to_time);

  // Act
  filter.Apply(ad_history);

  // Assert
  EXPECT_THAT(ad_history, ::testing::IsEmpty());
}

TEST(BraveAdsAdHistoryDateRangeFilterTest, FilterEmptyHistory) {
  // Arrange
  AdHistoryList ad_history;

  const base::Time from_time = base::Time::FromSecondsSinceUnixEpoch(444444444);
  const base::Time to_time = test::DistantFuture();

  const AdHistoryDateRangeFilter filter(from_time, to_time);

  // Act
  filter.Apply(ad_history);

  // Assert
  EXPECT_THAT(ad_history, ::testing::IsEmpty());
}

}  // namespace brave_ads
