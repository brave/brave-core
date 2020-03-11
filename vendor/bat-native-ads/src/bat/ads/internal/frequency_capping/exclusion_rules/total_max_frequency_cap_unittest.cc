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
#include "bat/ads/internal/frequency_capping/exclusion_rules/total_max_frequency_cap.h"

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

class BraveAdsTotalMaxFrequencyCapTest : public ::testing::Test {
 protected:
    BraveAdsTotalMaxFrequencyCapTest()
    : mock_ads_client_(std::make_unique<MockAdsClient>()),
      ads_(std::make_unique<AdsImpl>(mock_ads_client_.get())) {
    // You can do set-up work for each test here
  }

  ~BraveAdsTotalMaxFrequencyCapTest() override {
    // You can do clean-up work that doesn't throw exceptions here
  }

  // If the constructor and destructor are not enough for setting up and
  // cleaning up each test, you can use the following methods

  void SetUp() override {
    // Code here will be called immediately after the constructor (right before
    // each test)

    auto callback = std::bind(
        &BraveAdsTotalMaxFrequencyCapTest::OnAdsImplInitialize, this, _1);
    ads_->Initialize(callback);

    client_mock_ = std::make_unique<ClientMock>(ads_.get(),
        mock_ads_client_.get());
    frequency_capping_ = std::make_unique<FrequencyCapping>(client_mock_.get());
    exclusion_rule_ = std::make_unique<TotalMaxFrequencyCap>(
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
  std::unique_ptr<TotalMaxFrequencyCap> exclusion_rule_;
  CreativeAdNotificationInfo ad_notification_info_;
};

TEST_F(BraveAdsTotalMaxFrequencyCapTest, AdAllowedWithNoAdHistory) {
  // Arrange
  ad_notification_info_.creative_set_id = kTestCreativeSetIds.at(0);
  ad_notification_info_.total_max = 2;

  // Act
  const bool is_ad_excluded =
      exclusion_rule_->ShouldExclude(ad_notification_info_);

  // Assert
  EXPECT_FALSE(is_ad_excluded);
}

TEST_F(BraveAdsTotalMaxFrequencyCapTest, AdAllowedWithMatchingAds) {
  // Arrange
  ad_notification_info_.creative_set_id = kTestCreativeSetIds.at(0);
  ad_notification_info_.total_max = 2;

  client_mock_->GeneratePastCreativeSetHistoryFromNow(
      ad_notification_info_.creative_set_id, base::Time::kSecondsPerHour, 1);

  // Act
  const bool is_ad_excluded =
      exclusion_rule_->ShouldExclude(ad_notification_info_);

  // Assert
  EXPECT_FALSE(is_ad_excluded);
}

TEST_F(BraveAdsTotalMaxFrequencyCapTest, AdAllowedWithNonMatchingAds) {
  // Arrange
  client_mock_->GeneratePastCreativeSetHistoryFromNow(kTestCreativeSetIds.at(0),
      base::Time::kSecondsPerHour, 5);

  ad_notification_info_.creative_set_id = kTestCreativeSetIds.at(1);
  ad_notification_info_.total_max = 2;

  // Act
  const bool is_ad_excluded =
      exclusion_rule_->ShouldExclude(ad_notification_info_);

  // Assert
  EXPECT_FALSE(is_ad_excluded);
}

TEST_F(BraveAdsTotalMaxFrequencyCapTest, AdExcludedWhenNoneAllowed) {
  // Arrange
  ad_notification_info_.creative_set_id = kTestCreativeSetIds[0];
  ad_notification_info_.total_max = 0;

  client_mock_->GeneratePastCreativeSetHistoryFromNow(
      ad_notification_info_.creative_set_id, base::Time::kSecondsPerHour, 5);

  // Act
  const bool is_ad_excluded =
      exclusion_rule_->ShouldExclude(ad_notification_info_);

  // Assert
  EXPECT_TRUE(is_ad_excluded);
  EXPECT_EQ(exclusion_rule_->GetLastMessage(), "creativeSetId 654f10df-fbc4-4a92-8d43-2edf73734a60 has exceeded the frequency capping for totalMax");  // NOLINT
}

TEST_F(BraveAdsTotalMaxFrequencyCapTest, AdExcludedWhenMaximumReached) {
  // Arrange
  ad_notification_info_.creative_set_id = kTestCreativeSetIds.at(0);
  ad_notification_info_.total_max = 5;

  client_mock_->GeneratePastCreativeSetHistoryFromNow(
      ad_notification_info_.creative_set_id, base::Time::kSecondsPerHour, 5);

  // Act
  const bool is_ad_excluded =
      exclusion_rule_->ShouldExclude(ad_notification_info_);

  // Assert
  EXPECT_TRUE(is_ad_excluded);
  EXPECT_EQ(exclusion_rule_->GetLastMessage(), "creativeSetId 654f10df-fbc4-4a92-8d43-2edf73734a60 has exceeded the frequency capping for totalMax");  // NOLINT
}

}  // namespace ads
