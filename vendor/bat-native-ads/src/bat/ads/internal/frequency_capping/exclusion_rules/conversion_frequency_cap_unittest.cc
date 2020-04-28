/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <vector>

#include "testing/gtest/include/gtest/gtest.h"

#include "base/time/time.h"

#include "bat/ads/internal/frequency_capping/exclusion_rule.h"
#include "bat/ads/internal/frequency_capping/frequency_capping.h"
#include "bat/ads/internal/frequency_capping/exclusion_rules/conversion_frequency_cap.h"

#include "bat/ads/internal/client_mock.h"
#include "bat/ads/internal/ads_client_mock.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/creative_ad_notification_info.h"

// npm run test -- brave_unit_tests --filter=Ads*

using std::placeholders::_1;
using ::testing::_;
using ::testing::Invoke;

namespace {

const std::vector<std::string> kTestCreativeSetIds = {
    "654f10df-fbc4-4a92-8d43-2edf73734a60",
    "465f10df-fbc4-4a92-8d43-4edf73734a60"
};

}  // namespace

namespace ads {

class BraveAdsConversionFrequencyCapTest : public ::testing::Test {
 protected:
    BraveAdsConversionFrequencyCapTest()
    : mock_ads_client_(std::make_unique<MockAdsClient>()),
      ads_(std::make_unique<AdsImpl>(mock_ads_client_.get())) {
    // You can do set-up work for each test here
  }

  ~BraveAdsConversionFrequencyCapTest() override {
    // You can do clean-up work that doesn't throw exceptions here
  }

  // If the constructor and destructor are not enough for setting up and
  // cleaning up each test, you can use the following methods

  void SetUp() override {
    // Code here will be called immediately after the constructor (right before
    // each test)

    auto callback = std::bind(
        &BraveAdsConversionFrequencyCapTest::OnAdsImplInitialize, this, _1);
    ads_->Initialize(callback);

    client_mock_ = std::make_unique<ClientMock>(ads_.get(),
        mock_ads_client_.get());
    frequency_capping_ = std::make_unique<FrequencyCapping>(client_mock_.get());
    exclusion_rule_ = std::make_unique<ConversionFrequencyCap>(
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
  std::unique_ptr<ConversionFrequencyCap> exclusion_rule_;
  CreativeAdNotificationInfo ad_notification_info_;
};

TEST_F(BraveAdsConversionFrequencyCapTest, AdAllowedWithNoAdHistory) {
  // Arrange
  ad_notification_info_.creative_set_id = kTestCreativeSetIds.at(0);

  // Act
  const bool is_ad_excluded =
      exclusion_rule_->ShouldExclude(ad_notification_info_);

  // Assert
  EXPECT_FALSE(is_ad_excluded);
}

TEST_F(BraveAdsConversionFrequencyCapTest, AdNotAllowedWithMatchingAds) {
  // Arrange
  ad_notification_info_.creative_set_id = kTestCreativeSetIds.at(0);

  client_mock_->GeneratePastAdConversionHistoryFromNow(
      ad_notification_info_.creative_set_id, base::Time::kSecondsPerHour, 1);

  // Act
  const bool is_ad_excluded =
      exclusion_rule_->ShouldExclude(ad_notification_info_);

  // Assert
  EXPECT_TRUE(is_ad_excluded);
}

TEST_F(BraveAdsConversionFrequencyCapTest, AdAllowedWithNonMatchingAds) {
  // Arrange
  client_mock_->GeneratePastAdConversionHistoryFromNow(
      kTestCreativeSetIds.at(0), base::Time::kSecondsPerHour, 1);

  ad_notification_info_.creative_set_id = kTestCreativeSetIds.at(1);

  // Act
  const bool is_ad_excluded =
      exclusion_rule_->ShouldExclude(ad_notification_info_);

  // Assert
  EXPECT_FALSE(is_ad_excluded);
}

}  // namespace ads
