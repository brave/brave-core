/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads_history/filters/ads_history_date_range_filter.h"

#include <cstdint>
#include <limits>

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {

std::deque<AdHistoryInfo> GetAdsHistory() {
  std::deque<AdHistoryInfo> history;

  AdHistoryInfo ad_history;
  ad_history.timestamp_in_seconds = 33333333333;
  history.push_back(ad_history);
  ad_history.timestamp_in_seconds = 44444444444;
  history.push_back(ad_history);
  ad_history.timestamp_in_seconds = 22222222222;
  history.push_back(ad_history);
  ad_history.timestamp_in_seconds = 66666666666;
  history.push_back(ad_history);
  ad_history.timestamp_in_seconds = 55555555555;
  history.push_back(ad_history);

  return history;
}

}  // namespace

TEST(BatAdsHistoryDateRangeFilterTest,
     FilterHistoryFromTimestamp44444444444ToDistantFuture) {
  // Arrange
  std::deque<AdHistoryInfo> history = GetAdsHistory();

  const uint64_t from_timestamp = 44444444444;
  const uint64_t to_timestamp = std::numeric_limits<uint64_t>::max();

  // Act
  AdsHistoryDateRangeFilter filter;
  history = filter.Apply(history, from_timestamp, to_timestamp);

  // Assert
  std::deque<AdHistoryInfo> expected_history;

  AdHistoryInfo ad_history;
  ad_history.timestamp_in_seconds = 44444444444;
  expected_history.push_back(ad_history);
  ad_history.timestamp_in_seconds = 66666666666;
  expected_history.push_back(ad_history);
  ad_history.timestamp_in_seconds = 55555555555;
  expected_history.push_back(ad_history);

  EXPECT_EQ(expected_history, history);
}

TEST(BatAdsHistoryDateRangeFilterTest,
     FilterHistoryFromTimestamp77777777777ToDistantFuture) {
  // Arrange
  std::deque<AdHistoryInfo> history = GetAdsHistory();

  const uint64_t from_timestamp = 77777777777;
  const uint64_t to_timestamp = std::numeric_limits<uint64_t>::max();

  // Act
  AdsHistoryDateRangeFilter filter;
  history = filter.Apply(history, from_timestamp, to_timestamp);

  // Assert
  const std::deque<AdHistoryInfo> expected_history = {};

  EXPECT_EQ(expected_history, history);
}

TEST(BatAdsHistoryDateRangeFilterTest,
     FilterHistoryFromDistantPastToTimestamp44444444444) {
  // Arrange
  std::deque<AdHistoryInfo> history = GetAdsHistory();

  const uint64_t from_timestamp = std::numeric_limits<uint64_t>::min();
  const uint64_t to_timestamp = 44444444444;

  // Act
  AdsHistoryDateRangeFilter filter;
  history = filter.Apply(history, from_timestamp, to_timestamp);

  // Assert
  std::deque<AdHistoryInfo> expected_history;
  AdHistoryInfo ad_history;
  ad_history.timestamp_in_seconds = 33333333333;
  expected_history.push_back(ad_history);
  ad_history.timestamp_in_seconds = 44444444444;
  expected_history.push_back(ad_history);
  ad_history.timestamp_in_seconds = 22222222222;
  expected_history.push_back(ad_history);

  EXPECT_EQ(expected_history, history);
}

TEST(BatAdsHistoryDateRangeFilterTest,
     FilterHistoryFromDistancePastToTimestamp11111111111) {
  // Arrange
  std::deque<AdHistoryInfo> history = GetAdsHistory();

  const uint64_t from_timestamp = std::numeric_limits<uint64_t>::min();
  const uint64_t to_timestamp = 11111111111;

  // Act
  AdsHistoryDateRangeFilter filter;
  history = filter.Apply(history, from_timestamp, to_timestamp);

  // Assert
  const std::deque<AdHistoryInfo> expected_history = {};

  EXPECT_EQ(expected_history, history);
}

TEST(BatAdsHistoryDateRangeFilterTest,
     FilterHistoryFromDistantPastToDistantFuture) {
  // Arrange
  std::deque<AdHistoryInfo> history = GetAdsHistory();

  const uint64_t from_timestamp = std::numeric_limits<uint64_t>::min();
  const uint64_t to_timestamp = std::numeric_limits<uint64_t>::max();

  // Act
  AdsHistoryDateRangeFilter filter;
  history = filter.Apply(history, from_timestamp, to_timestamp);

  // Assert
  std::deque<AdHistoryInfo> expected_history;
  AdHistoryInfo ad_history;
  ad_history.timestamp_in_seconds = 33333333333;
  expected_history.push_back(ad_history);
  ad_history.timestamp_in_seconds = 44444444444;
  expected_history.push_back(ad_history);
  ad_history.timestamp_in_seconds = 22222222222;
  expected_history.push_back(ad_history);
  ad_history.timestamp_in_seconds = 66666666666;
  expected_history.push_back(ad_history);
  ad_history.timestamp_in_seconds = 55555555555;
  expected_history.push_back(ad_history);

  EXPECT_EQ(expected_history, history);
}

TEST(BatAdsHistoryDateRangeFilterTest,
     FilterHistoryFromDistantFutureToDistantPast) {
  // Arrange
  std::deque<AdHistoryInfo> history = GetAdsHistory();

  const uint64_t from_timestamp = std::numeric_limits<uint64_t>::max();
  const uint64_t to_timestamp = std::numeric_limits<uint64_t>::min();

  // Act
  AdsHistoryDateRangeFilter filter;
  history = filter.Apply(history, from_timestamp, to_timestamp);

  // Assert
  const std::deque<AdHistoryInfo> expected_history = {};

  EXPECT_EQ(expected_history, history);
}

TEST(BatAdsHistoryDateRangeFilterTest, FilterEmptyHistory) {
  // Arrange
  std::deque<AdHistoryInfo> history;

  const uint64_t from_timestamp = std::numeric_limits<uint64_t>::min();
  const uint64_t to_timestamp = std::numeric_limits<uint64_t>::max();

  // Act
  AdsHistoryDateRangeFilter filter;
  history = filter.Apply(history, from_timestamp, to_timestamp);

  // Assert
  const std::deque<AdHistoryInfo> expected_history = {};

  EXPECT_EQ(expected_history, history);
}

}  // namespace ads
