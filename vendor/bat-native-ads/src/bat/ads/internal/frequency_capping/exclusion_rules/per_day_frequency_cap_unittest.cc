/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/exclusion_rules/per_day_frequency_cap.h"  // NOLINT

#include <stdint.h>

#include <memory>

#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "brave/components/l10n/browser/locale_helper_mock.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "bat/ads/creative_ad_info.h"
#include "bat/ads/internal/ads_client_mock.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/frequency_capping/frequency_capping_unittest_utils.h"
#include "bat/ads/internal/ads_unittest_utils.h"

// npm run test -- brave_unit_tests --filter=BatAds*

using ::testing::NiceMock;
using ::testing::Return;

namespace ads {

namespace {

const char kCreativeSetId[] = "654f10df-fbc4-4a92-8d43-2edf73734a60";

const uint64_t kSecondsPerDay = base::Time::kSecondsPerHour *
    base::Time::kHoursPerDay;

}  // namespace


class BatAdsPerDayFrequencyCapTest : public ::testing::Test {
 protected:
  BatAdsPerDayFrequencyCapTest()
      : ads_client_mock_(std::make_unique<NiceMock<AdsClientMock>>()),
        ads_(std::make_unique<AdsImpl>(ads_client_mock_.get())),
        locale_helper_mock_(std::make_unique<NiceMock<
            brave_l10n::LocaleHelperMock>>()),
        frequency_cap_(std::make_unique<PerDayFrequencyCap>(ads_.get())) {
    // You can do set-up work for each test here

    brave_l10n::LocaleHelper::GetInstance()->set_for_testing(
        locale_helper_mock_.get());
  }

  ~BatAdsPerDayFrequencyCapTest() override {
    // You can do clean-up work that doesn't throw exceptions here
  }

  // If the constructor and destructor are not enough for setting up and
  // cleaning up each test, you can use the following methods

  void SetUp() override {
    // Code here will be called immediately after the constructor (right before
    // each test)

    ON_CALL(*ads_client_mock_, IsEnabled())
        .WillByDefault(Return(true));

    ON_CALL(*locale_helper_mock_, GetLocale())
        .WillByDefault(Return("en-US"));

    MockLoad(ads_client_mock_.get());
    MockLoadUserModelForLanguage(ads_client_mock_.get());
    MockLoadJsonSchema(ads_client_mock_.get());
    MockSave(ads_client_mock_.get());

    Initialize(ads_.get());
  }

  void TearDown() override {
    // Code here will be called immediately after each test (right before the
    // destructor)
  }

  // Objects declared here can be used by all tests in the test case

  base::test::TaskEnvironment task_environment_;

  std::unique_ptr<AdsClientMock> ads_client_mock_;
  std::unique_ptr<AdsImpl> ads_;
  std::unique_ptr<brave_l10n::LocaleHelperMock> locale_helper_mock_;
  std::unique_ptr<PerDayFrequencyCap> frequency_cap_;
};

TEST_F(BatAdsPerDayFrequencyCapTest,
    AllowAdIfThereIsNoAdsHistory) {
  // Arrange
  CreativeAdInfo ad;
  ad.creative_set_id = kCreativeSetId;
  ad.per_day = 2;

  // Act
  const bool should_exclude = frequency_cap_->ShouldExclude(ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsPerDayFrequencyCapTest,
    AllowAdIfDoesNotExceedCap) {
  // Arrange
  CreativeAdInfo ad;
  ad.creative_set_id = kCreativeSetId;
  ad.per_day = 2;

  GeneratePastCreativeSetHistoryFromNow(ads_->get_client(), kCreativeSetId,
      0, 1);

  // Act
  const bool should_exclude = frequency_cap_->ShouldExclude(ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsPerDayFrequencyCapTest,
    AllowAdIfDoesNotExceedCapAfter1Day) {
  // Arrange
  CreativeAdInfo ad;
  ad.creative_set_id = kCreativeSetId;
  ad.per_day = 2;

  GeneratePastCreativeSetHistoryFromNow(ads_->get_client(), kCreativeSetId,
      0, 1);

  GeneratePastCreativeSetHistoryFromNow(ads_->get_client(), kCreativeSetId,
      kSecondsPerDay, 1);

  // Act
  const bool should_exclude = frequency_cap_->ShouldExclude(ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsPerDayFrequencyCapTest,
    DoNotAllowAdIfExceedsCapWithin1Day) {
  // Arrange
  CreativeAdInfo ad;
  ad.creative_set_id = kCreativeSetId;
  ad.per_day = 2;

  GeneratePastCreativeSetHistoryFromNow(ads_->get_client(), kCreativeSetId,
      0, 1);
  GeneratePastCreativeSetHistoryFromNow(ads_->get_client(), kCreativeSetId,
      kSecondsPerDay - 1, 1);

  // Act
  const bool should_exclude = frequency_cap_->ShouldExclude(ad);

  // Assert
  EXPECT_TRUE(should_exclude);
}

TEST_F(BatAdsPerDayFrequencyCapTest,
    DoNotAllowAdIfExceedsCap) {
  // Arrange
  CreativeAdInfo ad;
  ad.creative_set_id = kCreativeSetId;
  ad.per_day = 2;

  GeneratePastCreativeSetHistoryFromNow(ads_->get_client(), kCreativeSetId,
      0, 2);

  // Act
  const bool should_exclude = frequency_cap_->ShouldExclude(ad);

  // Assert
  EXPECT_TRUE(should_exclude);
}

TEST_F(BatAdsPerDayFrequencyCapTest,
    DoNotAllowAdForIssue4207) {
  // Arrange
  const uint64_t ads_per_day = 20;
  const uint64_t ads_per_hour = 5;
  const uint64_t ad_interval = base::Time::kSecondsPerHour / ads_per_hour;

  CreativeAdInfo ad;
  ad.creative_set_id = kCreativeSetId;
  ad.per_day = ads_per_day;

  GeneratePastCreativeSetHistoryFromNow(ads_->get_client(), kCreativeSetId,
      ad_interval, ads_per_day);

  // Act
  const bool should_exclude = frequency_cap_->ShouldExclude(ad);

  // Assert
  EXPECT_TRUE(should_exclude);
}

}  // namespace ads
