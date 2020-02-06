/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "testing/gtest/include/gtest/gtest.h"

#include "base/time/time.h"

#include "bat/ads/internal/frequency_capping/exclusion_rule.h"
#include "bat/ads/internal/frequency_capping/frequency_capping.h"
#include "bat/ads/internal/frequency_capping/exclusion_rules/per_hour_frequency_cap.h"

#include "bat/ads/internal/client_mock.h"
#include "bat/ads/internal/ads_client_mock.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/creative_ad_notification_info.h"

// npm run test -- brave_unit_tests --filter=Ads*

using std::placeholders::_1;
using ::testing::_;
using ::testing::Invoke;

namespace {

const char kTestAdUuid[] = "9aea9a47-c6a0-4718-a0fa-706338bb2156";

}  // namespace

namespace ads {

class BraveAdsPerHourFrequencyCapTest : public ::testing::Test {
 protected:
  BraveAdsPerHourFrequencyCapTest()
  : mock_ads_client_(std::make_unique<MockAdsClient>()),
    ads_(std::make_unique<AdsImpl>(mock_ads_client_.get())) {
    // You can do set-up work for each test here
  }

  ~BraveAdsPerHourFrequencyCapTest() override {
    // You can do clean-up work that doesn't throw exceptions here
  }

  // If the constructor and destructor are not enough for setting up and
  // cleaning up each test, you can use the following methods

  void SetUp() override {
    // Code here will be called immediately after the constructor (right before
    // each test)

    auto callback = std::bind(
        &BraveAdsPerHourFrequencyCapTest::OnAdsImplInitialize, this, _1);
    ads_->Initialize(callback);

    client_mock_ = std::make_unique<ClientMock>(ads_.get(),
        mock_ads_client_.get());
    frequency_capping_ = std::make_unique<FrequencyCapping>(client_mock_.get());
    exclusion_rule_ = std::make_unique<PerHourFrequencyCap>(
        frequency_capping_.get());
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
  std::unique_ptr<PerHourFrequencyCap> exclusion_rule_;
  CreativeAdNotificationInfo ad_info_;
};

TEST_F(BraveAdsPerHourFrequencyCapTest, AdAllowedWhenNoAds) {
  // Arrange
  ad_info_.uuid = kTestAdUuid;

  // Act
  const bool is_ad_excluded = exclusion_rule_->ShouldExclude(ad_info_);

  // Assert
  EXPECT_FALSE(is_ad_excluded);
}

TEST_F(BraveAdsPerHourFrequencyCapTest, AdAllowedOverTheHour) {
  // Arrange
  ad_info_.uuid = kTestAdUuid;
  // 1hr 1s in the past
  client_mock_->GeneratePastAdHistoryFromNow(kTestAdUuid,
      base::Time::kSecondsPerHour, 1);

  // Act
  const bool is_ad_excluded = exclusion_rule_->ShouldExclude(ad_info_);

  // Assert
  EXPECT_FALSE(is_ad_excluded);
}

TEST_F(BraveAdsPerHourFrequencyCapTest,
    AdExcludedWithPastAdsJustWithinTheHour) {
  // Arrange
  ad_info_.uuid = kTestAdUuid;
  // 59m 59s
  client_mock_->GeneratePastAdHistoryFromNow(kTestAdUuid,
      base::Time::kSecondsPerHour - 1, 1);

  // Act
  const bool is_ad_excluded = exclusion_rule_->ShouldExclude(ad_info_);

  // Assert
  EXPECT_TRUE(is_ad_excluded);
  EXPECT_EQ(exclusion_rule_->GetLastMessage(), "adUUID 9aea9a47-c6a0-4718-a0fa-706338bb2156 has exceeded the frequency capping for perHour");  // NOLINT
}

TEST_F(BraveAdsPerHourFrequencyCapTest, AdExcludedWithPastAdWithinTheHour) {
  // Arrange
  ad_info_.uuid = kTestAdUuid;
  client_mock_->GeneratePastAdHistoryFromNow(kTestAdUuid, 0, 1);

  // Act
  const bool is_ad_excluded = exclusion_rule_->ShouldExclude(ad_info_);

  // Assert
  EXPECT_TRUE(is_ad_excluded);
  EXPECT_EQ(exclusion_rule_->GetLastMessage(), "adUUID 9aea9a47-c6a0-4718-a0fa-706338bb2156 has exceeded the frequency capping for perHour");  // NOLINT
}

}  // namespace ads
