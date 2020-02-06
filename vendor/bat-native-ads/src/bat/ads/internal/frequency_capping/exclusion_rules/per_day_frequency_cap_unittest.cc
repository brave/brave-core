/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "testing/gtest/include/gtest/gtest.h"

#include "bat/ads/internal/frequency_capping/exclusion_rule.h"
#include "bat/ads/internal/frequency_capping/frequency_capping.h"
#include "bat/ads/internal/frequency_capping/exclusion_rules/per_day_frequency_cap.h"

#include "bat/ads/internal/client_mock.h"
#include "bat/ads/internal/ads_client_mock.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/creative_ad_notification_info.h"
#include "bat/ads/internal/time.h"

// npm run test -- brave_unit_tests --filter=Ads*

using std::placeholders::_1;
using ::testing::_;
using ::testing::Invoke;

namespace {

const char kTestCreativeSetId[] = "654f10df-fbc4-4a92-8d43-2edf73734a60";

}  // namespace

namespace ads {

const uint64_t kSecondsPerDay = base::Time::kSecondsPerHour *
    base::Time::kHoursPerDay;

class BraveAdsPerDayFrequencyCapTest : public ::testing::Test {
 protected:
  BraveAdsPerDayFrequencyCapTest()
  : mock_ads_client_(std::make_unique<MockAdsClient>()),
    ads_(std::make_unique<AdsImpl>(mock_ads_client_.get())) {
    // You can do set-up work for each test here
  }

  ~BraveAdsPerDayFrequencyCapTest() override {
    // You can do clean-up work that doesn't throw exceptions here
  }

  // If the constructor and destructor are not enough for setting up and
  // cleaning up each test, you can use the following methods

  void SetUp() override {
    // Code here will be called immediately after the constructor (right before
    // each test)

    auto callback = std::bind(
        &BraveAdsPerDayFrequencyCapTest::OnAdsImplInitialize, this, _1);
    ads_->Initialize(callback);

    client_mock_ = std::make_unique<ClientMock>(ads_.get(),
        mock_ads_client_.get());
    frequency_capping_ = std::make_unique<FrequencyCapping>(client_mock_.get());
    exclusion_rule_ = std::make_unique<PerDayFrequencyCap>(
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
  std::unique_ptr<PerDayFrequencyCap> exclusion_rule_;
  CreativeAdNotificationInfo ad_info_;
};

TEST_F(BraveAdsPerDayFrequencyCapTest, AdAllowedWhenNoAds) {
  // Arrange
  ad_info_.creative_set_id = kTestCreativeSetId;
  ad_info_.per_day = 2;

  // Act
  const bool is_ad_excluded = exclusion_rule_->ShouldExclude(ad_info_);

  // Assert
  EXPECT_FALSE(is_ad_excluded);
}

TEST_F(BraveAdsPerDayFrequencyCapTest, AdAllowedBelowDailyCap) {
  // Arrange
  ad_info_.creative_set_id = kTestCreativeSetId;
  ad_info_.per_day = 2;
  client_mock_->GeneratePastCreativeSetHistoryFromNow(
      kTestCreativeSetId, 0, 1);

  // Act
  const bool is_ad_excluded = exclusion_rule_->ShouldExclude(ad_info_);

  // Assert
  EXPECT_FALSE(is_ad_excluded);
}

TEST_F(BraveAdsPerDayFrequencyCapTest, AdAllowedWithAdOutsideDayWindow) {
  // Arrange
  ad_info_.creative_set_id = kTestCreativeSetId;
  ad_info_.per_day = 2;

  client_mock_->GeneratePastCreativeSetHistoryFromNow(
      kTestCreativeSetId, 0, 1);
  client_mock_->GeneratePastCreativeSetHistoryFromNow(
      kTestCreativeSetId, kSecondsPerDay, 1);

  // Act
  const bool is_ad_excluded = exclusion_rule_->ShouldExclude(ad_info_);

  // Assert
  EXPECT_FALSE(is_ad_excluded);
}

TEST_F(BraveAdsPerDayFrequencyCapTest, AdExcludedAboveDailyCapWithRecentAds) {
  // Arrange
  ad_info_.creative_set_id = kTestCreativeSetId;
  ad_info_.per_day = 2;

  client_mock_->GeneratePastCreativeSetHistoryFromNow(
      kTestCreativeSetId, 0, 2);

  // Act
  const bool is_ad_excluded = exclusion_rule_->ShouldExclude(ad_info_);

  // Assert
  EXPECT_TRUE(is_ad_excluded);
  EXPECT_EQ(exclusion_rule_->GetLastMessage(), "creativeSetId 654f10df-fbc4-4a92-8d43-2edf73734a60 has exceeded the frequency capping for perDay");  // NOLINT
}

TEST_F(BraveAdsPerDayFrequencyCapTest,
    AdExcludedAboveDailyCapWithAdsJustWithinDay) {
  // Arrange
  ad_info_.creative_set_id = kTestCreativeSetId;
  ad_info_.per_day = 2;

  client_mock_->GeneratePastCreativeSetHistoryFromNow(kTestCreativeSetId, 0,
    1);
  client_mock_->GeneratePastCreativeSetHistoryFromNow(kTestCreativeSetId,
    kSecondsPerDay - 1, 1);

  // Act
  const bool is_ad_excluded = exclusion_rule_->ShouldExclude(ad_info_);

  // Assert
  EXPECT_TRUE(is_ad_excluded);
  EXPECT_EQ(exclusion_rule_->GetLastMessage(), "creativeSetId 654f10df-fbc4-4a92-8d43-2edf73734a60 has exceeded the frequency capping for perDay");  // NOLINT
}

TEST_F(BraveAdsPerDayFrequencyCapTest, AdExcludedForIssue4207) {
  // Arrange
  uint64_t ads_per_day = 20;

  ad_info_.creative_set_id = kTestCreativeSetId;
  ad_info_.per_day = ads_per_day;

  uint64_t ads_per_hour = 5;
  uint64_t ad_interval = base::Time::kSecondsPerHour / ads_per_hour;

  client_mock_->GeneratePastCreativeSetHistoryFromNow(kTestCreativeSetId,
      ad_interval, ads_per_day);

  // Act
  const bool is_ad_excluded = exclusion_rule_->ShouldExclude(ad_info_);

  // Assert
  EXPECT_TRUE(is_ad_excluded);
  EXPECT_EQ(exclusion_rule_->GetLastMessage(), "creativeSetId 654f10df-fbc4-4a92-8d43-2edf73734a60 has exceeded the frequency capping for perDay");  // NOLINT
}

}  // namespace ads
