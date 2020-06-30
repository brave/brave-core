/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/permission_rules/ads_per_hour_frequency_cap.h"

#include <memory>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "brave/components/l10n/browser/locale_helper_mock.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "bat/ads/internal/ads_client_mock.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/creative_ad_info.h"
#include "bat/ads/internal/frequency_capping/frequency_capping_unittest_utils.h"
#include "bat/ads/internal/unittest_utils.h"

// npm run test -- brave_unit_tests --filter=BatAds*

using ::testing::NiceMock;
using ::testing::Return;

namespace ads {

namespace {

const char kCreativeInstanceId[] = "9aea9a47-c6a0-4718-a0fa-706338bb2156";

}  // namespace

class BatAdsAdsPerHourFrequencyCapTest : public ::testing::Test {
 protected:
  BatAdsAdsPerHourFrequencyCapTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME),
        ads_client_mock_(std::make_unique<NiceMock<AdsClientMock>>()),
        ads_(std::make_unique<AdsImpl>(ads_client_mock_.get())),
        locale_helper_mock_(std::make_unique<NiceMock<
            brave_l10n::LocaleHelperMock>>()),
        frequency_cap_(std::make_unique<AdsPerHourFrequencyCap>(ads_.get())) {
    // You can do set-up work for each test here

    brave_l10n::LocaleHelper::GetInstance()->set_for_testing(
        locale_helper_mock_.get());
  }

  ~BatAdsAdsPerHourFrequencyCapTest() override {
    // You can do clean-up work that doesn't throw exceptions here
  }

  // If the constructor and destructor are not enough for setting up and
  // cleaning up each test, you can use the following methods

  void SetUp() override {
    // Code here will be called immediately after the constructor (right before
    // each test)

    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    const base::FilePath path = temp_dir_.GetPath();

    ON_CALL(*ads_client_mock_, IsEnabled())
        .WillByDefault(Return(true));

    ON_CALL(*locale_helper_mock_, GetLocale())
        .WillByDefault(Return("en-US"));

    MockLoad(ads_client_mock_);
    MockLoadUserModelForLanguage(ads_client_mock_);
    MockLoadJsonSchema(ads_client_mock_);
    MockSave(ads_client_mock_);

    database_ = std::make_unique<Database>(path.AppendASCII("database.sqlite"));
    MockRunDBTransaction(ads_client_mock_, database_);

    Initialize(ads_);
  }

  void TearDown() override {
    // Code here will be called immediately after each test (right before the
    // destructor)
  }

  // Objects declared here can be used by all tests in the test case

  base::test::TaskEnvironment task_environment_;

  base::ScopedTempDir temp_dir_;

  std::unique_ptr<AdsClientMock> ads_client_mock_;
  std::unique_ptr<AdsImpl> ads_;
  std::unique_ptr<brave_l10n::LocaleHelperMock> locale_helper_mock_;
  std::unique_ptr<AdsPerHourFrequencyCap> frequency_cap_;
  std::unique_ptr<Database> database_;
};

TEST_F(BatAdsAdsPerHourFrequencyCapTest,
    AllowAdIfThereIsNoAdsHistory) {
  // Arrange
  ON_CALL(*ads_client_mock_, GetAdsPerHour())
      .WillByDefault(Return(2));

  // Act
  const bool is_allowed = frequency_cap_->IsAllowed();

  // Assert
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsAdsPerHourFrequencyCapTest,
    AlwaysAllowAdOnMobileDevices) {
  // Arrange
  ON_CALL(*ads_client_mock_, GetAdsPerHour())
      .WillByDefault(Return(2));

  MockGetClientInfo(ads_client_mock_, ClientInfoPlatformType::IOS);

  GeneratePastAdsHistoryFromNow(ads_, kCreativeInstanceId,
      base::Time::kSecondsPerHour - 1, 1);

  // Act
  const bool is_allowed = frequency_cap_->IsAllowed();

  // Assert
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsAdsPerHourFrequencyCapTest,
    AllowAdIfDoesNotExceedCap) {
  // Arrange
  ON_CALL(*ads_client_mock_, GetAdsPerHour())
      .WillByDefault(Return(2));

  GeneratePastAdsHistoryFromNow(ads_, kCreativeInstanceId,
      base::Time::kSecondsPerHour - 1, 1);

  // Act
  const bool is_allowed = frequency_cap_->IsAllowed();

  // Assert
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsAdsPerHourFrequencyCapTest,
    AllowAdIfDoesNotExceedCapAfter1Hour) {
  // Arrange
  ON_CALL(*ads_client_mock_, GetAdsPerHour())
      .WillByDefault(Return(2));

  GeneratePastAdsHistoryFromNow(ads_, kCreativeInstanceId,
      base::Time::kSecondsPerHour, 2);

  // Act
  const bool is_allowed = frequency_cap_->IsAllowed();

  // Assert
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsAdsPerHourFrequencyCapTest,
    DoNotAllowAdIfExceedsCap) {
  // Arrange
  ON_CALL(*ads_client_mock_, GetAdsPerHour())
      .WillByDefault(Return(2));

  GeneratePastAdsHistoryFromNow(ads_, kCreativeInstanceId,
      10 * base::Time::kSecondsPerMinute, 2);

  // Act
  const bool is_allowed = frequency_cap_->IsAllowed();

  // Assert
  EXPECT_FALSE(is_allowed);
}

}  // namespace ads
