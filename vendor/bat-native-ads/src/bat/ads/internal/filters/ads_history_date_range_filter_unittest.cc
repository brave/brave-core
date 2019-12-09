/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <limits>
#include <memory>

#include "bat/ads/internal/filters/ads_history_date_range_filter.h"
#include "bat/ads/internal/client_mock.h"
#include "bat/ads/internal/ads_client_mock.h"
#include "bat/ads/internal/ads_impl.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

using std::placeholders::_1;

namespace ads {

class BatAdsHistoryDateRangeFilterTest : public ::testing::Test {
 protected:
  BatAdsHistoryDateRangeFilterTest()
      : mock_ads_client_(std::make_unique<MockAdsClient>()),
        ads_(std::make_unique<AdsImpl>(mock_ads_client_.get())) {
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

    auto callback = std::bind(
        &BatAdsHistoryDateRangeFilterTest::OnAdsImplInitialize, this, _1);
    ads_->Initialize(callback);

    client_mock_ =
        std::make_unique<ClientMock>(ads_.get(), mock_ads_client_.get());

    filter_ = std::make_unique<AdsHistoryDateRangeFilter>();
  }

  void TearDown() override {
    // Code here will be called immediately after each test (right before the
    // destructor)
  }

  void OnAdsImplInitialize(const Result result) {
    EXPECT_EQ(Result::SUCCESS, result);
  }

  std::deque<AdHistory> GetHistory() {
    AdContent ad_content;
    CategoryContent category_content;

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

  std::unique_ptr<MockAdsClient> mock_ads_client_;
  std::unique_ptr<AdsImpl> ads_;

  std::unique_ptr<ClientMock> client_mock_;

  std::unique_ptr<AdsHistoryDateRangeFilter> filter_;
};

TEST_F(BatAdsHistoryDateRangeFilterTest,
    FilterHistoryFromTimestamp44444444444ToDistantFuture) {
  // Arrange
  std::deque<AdHistory> expected_history;
  AdHistory ad_history;
  ad_history.timestamp_in_seconds = 44444444444;
  expected_history.push_back(ad_history);
  ad_history.timestamp_in_seconds = 66666666666;
  expected_history.push_back(ad_history);
  ad_history.timestamp_in_seconds = 55555555555;
  expected_history.push_back(ad_history);

  auto history = GetHistory();
  const uint64_t from_timestamp = 44444444444;
  const uint64_t to_timestamp = std::numeric_limits<uint64_t>::max();

  // Act
  history = filter_->Apply(history, from_timestamp, to_timestamp);

  // Assert
  ASSERT_EQ(expected_history, history);
}

TEST_F(BatAdsHistoryDateRangeFilterTest,
    FilterHistoryFromTimestamp77777777777ToDistantFuture) {
  // Arrange
  std::deque<AdHistory> expected_history;

  auto history = GetHistory();
  const uint64_t from_timestamp = 77777777777;
  const uint64_t to_timestamp = std::numeric_limits<uint64_t>::max();

  // Act
  history = filter_->Apply(history, from_timestamp, to_timestamp);

  // Assert
  ASSERT_EQ(expected_history, history);
}

TEST_F(BatAdsHistoryDateRangeFilterTest,
    FilterHistoryFromDistantPastToTimestamp44444444444) {
  // Arrange
  std::deque<AdHistory> expected_history;
  AdHistory ad_history;
  ad_history.timestamp_in_seconds = 33333333333;
  expected_history.push_back(ad_history);
  ad_history.timestamp_in_seconds = 44444444444;
  expected_history.push_back(ad_history);
  ad_history.timestamp_in_seconds = 22222222222;
  expected_history.push_back(ad_history);

  auto history = GetHistory();
  const uint64_t from_timestamp = std::numeric_limits<uint64_t>::min();
  const uint64_t to_timestamp = 44444444444;

  // Act
  history = filter_->Apply(history, from_timestamp, to_timestamp);

  // Assert
  ASSERT_EQ(expected_history, history);
}

TEST_F(BatAdsHistoryDateRangeFilterTest,
    FilterHistoryFromDistancePastToTimestamp11111111111) {
  // Arrange
  std::deque<AdHistory> expected_history;

  auto history = GetHistory();
  const uint64_t from_timestamp = std::numeric_limits<uint64_t>::min();
  const uint64_t to_timestamp = 11111111111;

  // Act
  history = filter_->Apply(history, from_timestamp, to_timestamp);

  // Assert
  ASSERT_EQ(expected_history, history);
}

TEST_F(BatAdsHistoryDateRangeFilterTest,
    FilterHistoryFromDistantPastToDistantFuture) {
  // Arrange
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

  auto history = GetHistory();
  const uint64_t from_timestamp = std::numeric_limits<uint64_t>::min();
  const uint64_t to_timestamp = std::numeric_limits<uint64_t>::max();

  // Act
  history = filter_->Apply(history, from_timestamp, to_timestamp);

  // Assert
  ASSERT_EQ(expected_history, history);
}

TEST_F(BatAdsHistoryDateRangeFilterTest,
    FilterHistoryFromDistantFutureToDistantPast) {
  // Arrange
  std::deque<AdHistory> expected_history;

  auto history = GetHistory();
  const uint64_t from_timestamp = std::numeric_limits<uint64_t>::max();
  const uint64_t to_timestamp = std::numeric_limits<uint64_t>::min();

  // Act
  history = filter_->Apply(history, from_timestamp, to_timestamp);

  // Assert
  ASSERT_EQ(expected_history, history);
}

TEST_F(BatAdsHistoryDateRangeFilterTest,
    FilterEmptyHistory) {
  // Arrange
  std::deque<AdHistory> expected_history;
  std::deque<AdHistory> history;

  const uint64_t from_timestamp = std::numeric_limits<uint64_t>::min();
  const uint64_t to_timestamp = std::numeric_limits<uint64_t>::max();

  // Act
  history = filter_->Apply(history, from_timestamp, to_timestamp);

  // Assert
  ASSERT_EQ(expected_history, history);
}

}  // namespace ads
