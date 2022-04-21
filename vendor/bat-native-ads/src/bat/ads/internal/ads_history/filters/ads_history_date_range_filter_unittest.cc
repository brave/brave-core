/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads_history/filters/ads_history_date_range_filter.h"

#include "base/containers/circular_deque.h"
#include "base/time/time.h"
#include "bat/ads/ad_history_info.h"
#include "bat/ads/ads_history_info.h"
#include "bat/ads/internal/container_util.h"
#include "bat/ads/internal/unittest_time_util.h"
#include "bat/ads/internal/unittest_util.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {

base::circular_deque<AdHistoryInfo> GetAdsHistory() {
  base::circular_deque<AdHistoryInfo> history;

  AdHistoryInfo ad_history;
  ad_history.timestamp = 33333333333;
  history.push_back(ad_history);
  ad_history.timestamp = 44444444444;
  history.push_back(ad_history);
  ad_history.timestamp = 22222222222;
  history.push_back(ad_history);
  ad_history.timestamp = 66666666666;
  history.push_back(ad_history);
  ad_history.timestamp = 55555555555;
  history.push_back(ad_history);

  return history;
}

}  // namespace

TEST(BatAdsHistoryDateRangeFilterTest,
     FilterHistoryFromTimestamp44444444444ToDistantFuture) {
  // Arrange
  base::circular_deque<AdHistoryInfo> history = GetAdsHistory();

  const base::Time from_time = TimestampToTime(44444444444);
  const base::Time to_time = MaxTime();

  // Act
  AdsHistoryDateRangeFilter filter;
  history = filter.Apply(history, from_time, to_time);

  // Assert
  base::circular_deque<AdHistoryInfo> expected_history;

  AdHistoryInfo ad_history;
  ad_history.timestamp = 44444444444;
  expected_history.push_back(ad_history);
  ad_history.timestamp = 66666666666;
  expected_history.push_back(ad_history);
  ad_history.timestamp = 55555555555;
  expected_history.push_back(ad_history);

  EXPECT_TRUE(IsEqualContainers(expected_history, history));
}

TEST(BatAdsHistoryDateRangeFilterTest,
     FilterHistoryFromTimestamp77777777777ToDistantFuture) {
  // Arrange
  base::circular_deque<AdHistoryInfo> history = GetAdsHistory();

  const base::Time from_time = TimestampToTime(77777777777);
  const base::Time to_time = MaxTime();

  // Act
  AdsHistoryDateRangeFilter filter;
  history = filter.Apply(history, from_time, to_time);

  // Assert
  const base::circular_deque<AdHistoryInfo> expected_history = {};

  EXPECT_TRUE(IsEqualContainers(expected_history, history));
}

TEST(BatAdsHistoryDateRangeFilterTest,
     FilterHistoryFromDistantPastToTimestamp44444444444) {
  // Arrange
  base::circular_deque<AdHistoryInfo> history = GetAdsHistory();

  const base::Time from_time = MinTime();
  const base::Time to_time = TimestampToTime(44444444444);

  // Act
  AdsHistoryDateRangeFilter filter;
  history = filter.Apply(history, from_time, to_time);

  // Assert
  base::circular_deque<AdHistoryInfo> expected_history;
  AdHistoryInfo ad_history;
  ad_history.timestamp = 33333333333;
  expected_history.push_back(ad_history);
  ad_history.timestamp = 44444444444;
  expected_history.push_back(ad_history);
  ad_history.timestamp = 22222222222;
  expected_history.push_back(ad_history);

  EXPECT_TRUE(IsEqualContainers(expected_history, history));
}

TEST(BatAdsHistoryDateRangeFilterTest,
     FilterHistoryFromDistancePastToTimestamp11111111111) {
  // Arrange
  base::circular_deque<AdHistoryInfo> history = GetAdsHistory();

  const base::Time from_time = MinTime();
  const base::Time to_time = TimestampToTime(11111111111);

  // Act
  AdsHistoryDateRangeFilter filter;
  history = filter.Apply(history, from_time, to_time);

  // Assert
  const base::circular_deque<AdHistoryInfo> expected_history = {};

  EXPECT_TRUE(IsEqualContainers(expected_history, history));
}

TEST(BatAdsHistoryDateRangeFilterTest,
     FilterHistoryFromDistantPastToDistantFuture) {
  // Arrange
  base::circular_deque<AdHistoryInfo> history = GetAdsHistory();

  const base::Time from_time = MinTime();
  const base::Time to_time = MaxTime();

  // Act
  AdsHistoryDateRangeFilter filter;
  history = filter.Apply(history, from_time, to_time);

  // Assert
  base::circular_deque<AdHistoryInfo> expected_history;
  AdHistoryInfo ad_history;
  ad_history.timestamp = 33333333333;
  expected_history.push_back(ad_history);
  ad_history.timestamp = 44444444444;
  expected_history.push_back(ad_history);
  ad_history.timestamp = 22222222222;
  expected_history.push_back(ad_history);
  ad_history.timestamp = 66666666666;
  expected_history.push_back(ad_history);
  ad_history.timestamp = 55555555555;
  expected_history.push_back(ad_history);

  EXPECT_TRUE(IsEqualContainers(expected_history, history));
}

TEST(BatAdsHistoryDateRangeFilterTest,
     FilterHistoryFromDistantFutureToDistantPast) {
  // Arrange
  base::circular_deque<AdHistoryInfo> history = GetAdsHistory();

  const base::Time from_time = MaxTime();
  const base::Time to_time = MinTime();

  // Act
  AdsHistoryDateRangeFilter filter;
  history = filter.Apply(history, from_time, to_time);

  // Assert
  const base::circular_deque<AdHistoryInfo> expected_history = {};

  EXPECT_TRUE(IsEqualContainers(expected_history, history));
}

TEST(BatAdsHistoryDateRangeFilterTest, FilterEmptyHistory) {
  // Arrange
  base::circular_deque<AdHistoryInfo> history;

  const base::Time from_time = TimestampToTime(44444444444);
  const base::Time to_time = MaxTime();

  // Act
  AdsHistoryDateRangeFilter filter;
  history = filter.Apply(history, from_time, to_time);

  // Assert
  const base::circular_deque<AdHistoryInfo> expected_history = {};

  EXPECT_TRUE(IsEqualContainers(expected_history, history));
}

}  // namespace ads
