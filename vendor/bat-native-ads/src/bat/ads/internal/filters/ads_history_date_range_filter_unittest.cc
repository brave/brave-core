/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/filters/ads_history_date_range_filter.h"

#include <stdint.h>

#include <deque>
#include <limits>
#include <memory>

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "bat/ads/ad_history.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsHistoryDateRangeFilterTest : public ::testing::Test {
 protected:
  BatAdsHistoryDateRangeFilterTest()
      : filter_(std::make_unique<AdsHistoryDateRangeFilter>()) {
    // You can do set-up work for each test here
  }

  ~BatAdsHistoryDateRangeFilterTest() override {
    // You can do clean-up work that doesn't throw exceptions here
  }

  // If the constructor and destructor are not enough for setting up and
  // cleaning up each test, you can use the following methods

  void SetUp() override {
    // Code here will be called immediately after the constructor (right before
    // each test)
  }

  void TearDown() override {
    // Code here will be called immediately after each test (right before the
    // destructor)
  }

  // Objects declared here can be used by all tests in the test case

  std::deque<AdHistory> GetAdsHistory() {
    std::deque<AdHistory> history;

    AdHistory ad_history;
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

  std::unique_ptr<AdsHistoryDateRangeFilter> filter_;
};

TEST_F(BatAdsHistoryDateRangeFilterTest,
    FilterHistoryFromTimestamp44444444444ToDistantFuture) {
  // Arrange
  std::deque<AdHistory> history = GetAdsHistory();

  const uint64_t from_timestamp = 44444444444;
  const uint64_t to_timestamp = std::numeric_limits<uint64_t>::max();

  // Act
  history = filter_->Apply(history, from_timestamp, to_timestamp);

  // Assert
  std::deque<AdHistory> expected_history;

  AdHistory ad_history;
  ad_history.timestamp_in_seconds = 44444444444;
  expected_history.push_back(ad_history);
  ad_history.timestamp_in_seconds = 66666666666;
  expected_history.push_back(ad_history);
  ad_history.timestamp_in_seconds = 55555555555;
  expected_history.push_back(ad_history);

  EXPECT_EQ(expected_history, history);
}

TEST_F(BatAdsHistoryDateRangeFilterTest,
    FilterHistoryFromTimestamp77777777777ToDistantFuture) {
  // Arrange
  std::deque<AdHistory> history = GetAdsHistory();

  const uint64_t from_timestamp = 77777777777;
  const uint64_t to_timestamp = std::numeric_limits<uint64_t>::max();

  // Act
  const std::deque<AdHistory> expected_history = {};

  history = filter_->Apply(history, from_timestamp, to_timestamp);

  // Assert
  EXPECT_EQ(expected_history, history);
}

TEST_F(BatAdsHistoryDateRangeFilterTest,
    FilterHistoryFromDistantPastToTimestamp44444444444) {
  // Arrange
  std::deque<AdHistory> history = GetAdsHistory();

  const uint64_t from_timestamp = std::numeric_limits<uint64_t>::min();
  const uint64_t to_timestamp = 44444444444;

  // Act
  history = filter_->Apply(history, from_timestamp, to_timestamp);

  // Assert
  std::deque<AdHistory> expected_history;
  AdHistory ad_history;
  ad_history.timestamp_in_seconds = 33333333333;
  expected_history.push_back(ad_history);
  ad_history.timestamp_in_seconds = 44444444444;
  expected_history.push_back(ad_history);
  ad_history.timestamp_in_seconds = 22222222222;
  expected_history.push_back(ad_history);

  EXPECT_EQ(expected_history, history);
}

TEST_F(BatAdsHistoryDateRangeFilterTest,
    FilterHistoryFromDistancePastToTimestamp11111111111) {
  // Arrange
  std::deque<AdHistory> history = GetAdsHistory();

  const uint64_t from_timestamp = std::numeric_limits<uint64_t>::min();
  const uint64_t to_timestamp = 11111111111;

  // Act
  history = filter_->Apply(history, from_timestamp, to_timestamp);

  // Assert
  const std::deque<AdHistory> expected_history = {};

  EXPECT_EQ(expected_history, history);
}

TEST_F(BatAdsHistoryDateRangeFilterTest,
    FilterHistoryFromDistantPastToDistantFuture) {
  // Arrange
  std::deque<AdHistory> history = GetAdsHistory();

  const uint64_t from_timestamp = std::numeric_limits<uint64_t>::min();
  const uint64_t to_timestamp = std::numeric_limits<uint64_t>::max();

  // Act
  history = filter_->Apply(history, from_timestamp, to_timestamp);

  // Assert
  std::deque<AdHistory> expected_history;
  AdHistory ad_history;
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

TEST_F(BatAdsHistoryDateRangeFilterTest,
    FilterHistoryFromDistantFutureToDistantPast) {
  // Arrange
  std::deque<AdHistory> history = GetAdsHistory();

  const uint64_t from_timestamp = std::numeric_limits<uint64_t>::max();
  const uint64_t to_timestamp = std::numeric_limits<uint64_t>::min();

  // Act
  history = filter_->Apply(history, from_timestamp, to_timestamp);

  // Assert
  const std::deque<AdHistory> expected_history = {};

  EXPECT_EQ(expected_history, history);
}

TEST_F(BatAdsHistoryDateRangeFilterTest,
    FilterEmptyHistory) {
  // Arrange
  std::deque<AdHistory> history;

  const uint64_t from_timestamp = std::numeric_limits<uint64_t>::min();
  const uint64_t to_timestamp = std::numeric_limits<uint64_t>::max();

  // Act
  history = filter_->Apply(history, from_timestamp, to_timestamp);

  // Assert
  const std::deque<AdHistory> expected_history = {};

  EXPECT_EQ(expected_history, history);
}

}  // namespace ads
