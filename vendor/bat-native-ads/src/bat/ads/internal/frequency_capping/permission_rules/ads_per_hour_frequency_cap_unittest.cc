/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "testing/gtest/include/gtest/gtest.h"

#include "base/time/time.h"

#include "bat/ads/internal/frequency_capping/permission_rules/ads_per_hour_frequency_cap.h"
#include "bat/ads/internal/frequency_capping/frequency_capping.h"

#include "bat/ads/internal/client_mock.h"
#include "bat/ads/internal/ads_client_mock.h"
#include "bat/ads/internal/ads_impl.h"

// npm run test -- brave_unit_tests --filter=Ads*

using std::placeholders::_1;
using ::testing::_;
using ::testing::Invoke;

namespace {

const char kTestAdCreativeInstanceId[] = "9aea9a47-c6a0-4718-a0fa-706338bb2156";

}  // namespace

namespace ads {

class BraveAdsAdsPerHourFrequencyCapTest : public ::testing::Test {
 protected:
  BraveAdsAdsPerHourFrequencyCapTest()
  : mock_ads_client_(std::make_unique<MockAdsClient>()),
    ads_(std::make_unique<AdsImpl>(mock_ads_client_.get())) {
    // You can do set-up work for each test here
  }

  ~BraveAdsAdsPerHourFrequencyCapTest() override {
    // You can do clean-up work that doesn't throw exceptions here
  }

  // If the constructor and destructor are not enough for setting up and
  // cleaning up each test, you can use the following methods

  void SetUp() override {
    // Code here will be called immediately after the constructor (right before
    // each test)

    auto callback = std::bind(
        &BraveAdsAdsPerHourFrequencyCapTest::OnAdsImplInitialize, this, _1);
    ads_->Initialize(callback);

    client_mock_ = std::make_unique<ClientMock>(ads_.get(),
        mock_ads_client_.get());
    frequency_capping_ = std::make_unique<FrequencyCapping>(
        client_mock_.get());
    per_hour_limit_ = std::make_unique<AdsPerHourFrequencyCap>(
        ads_.get(), mock_ads_client_.get(), frequency_capping_.get());
  }

  void OnAdsImplInitialize(const Result result) {
    EXPECT_EQ(Result::SUCCESS, result);
  }

  void TearDown() override {
    // Code here will be called immediately after each test (right before the
    // destructor)
  }

  std::unique_ptr<MockAdsClient> mock_ads_client_;
  std::unique_ptr<AdsImpl> ads_;

  std::unique_ptr<ClientMock> client_mock_;
  std::unique_ptr<FrequencyCapping> frequency_capping_;
  std::unique_ptr<AdsPerHourFrequencyCap> per_hour_limit_;
};

TEST_F(BraveAdsAdsPerHourFrequencyCapTest,
    PerHourLimitRespectedWithNoAdHistory) {
  // Arrange
  ON_CALL(*mock_ads_client_, GetAdsPerHour())
      .WillByDefault(testing::Return(2));

  // Act
  const bool does_history_respect_per_hour_limit =
       per_hour_limit_->IsAllowed();

  // Assert
  EXPECT_TRUE(does_history_respect_per_hour_limit);
}

TEST_F(BraveAdsAdsPerHourFrequencyCapTest,
    PerHourLimitRespectedWithAdsBelowPerHourLimit) {
  // Arrange
  ON_CALL(*mock_ads_client_, GetAdsPerHour())
      .WillByDefault(testing::Return(2));

  client_mock_->GeneratePastAdHistoryFromNow(kTestAdCreativeInstanceId,
    base::Time::kSecondsPerHour - 1, 1);

  // Act
  const bool does_history_respect_per_hour_limit =
      per_hour_limit_->IsAllowed();

  // Assert
  EXPECT_TRUE(does_history_respect_per_hour_limit);
}

TEST_F(BraveAdsAdsPerHourFrequencyCapTest,
    PerHourLimitRespectedWithAdsAbovePerHourLimitButOlderThanAnHour) {
  // Arrange
  ON_CALL(*mock_ads_client_, GetAdsPerHour())
      .WillByDefault(testing::Return(2));

  client_mock_->GeneratePastAdHistoryFromNow(kTestAdCreativeInstanceId,
      base::Time::kSecondsPerHour, 2);

  // Act
  const bool does_history_respect_per_hour_limit =
      per_hour_limit_->IsAllowed();

  // Assert
  EXPECT_TRUE(does_history_respect_per_hour_limit);
}

TEST_F(BraveAdsAdsPerHourFrequencyCapTest,
    PerHourLimitNotRespectedWithAdsAbovePerHourLimit) {
  // Arrange
  ON_CALL(*mock_ads_client_, GetAdsPerHour())
      .WillByDefault(testing::Return(2));

  client_mock_->GeneratePastAdHistoryFromNow(
      kTestAdCreativeInstanceId, 10 * 60, 2);

  // Act
  const bool does_history_respect_per_hour_limit =
      per_hour_limit_->IsAllowed();

  // Assert
  EXPECT_FALSE(does_history_respect_per_hour_limit);
  EXPECT_EQ(per_hour_limit_->GetLastMessage(), "You have exceeded the allowed ads per hour");  // NOLINT
}

}  // namespace ads
