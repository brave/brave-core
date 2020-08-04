/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/database/tables/ad_conversions_database_table.h"

#include <memory>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/test/task_environment.h"
#include "brave/components/l10n/browser/locale_helper_mock.h"
#include "net/http/http_status_code.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "bat/ads/internal/ad_conversions/ad_conversion_info.h"
#include "bat/ads/internal/ads_client_mock.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/container_util.h"
#include "bat/ads/internal/database/database_initialize.h"
#include "bat/ads/internal/platform/platform_helper_mock.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

using ::testing::NiceMock;
using ::testing::Return;

namespace ads {

class BatAdsAdConversionsDatabaseTableTest : public ::testing::Test {
 protected:
  BatAdsAdConversionsDatabaseTableTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME),
        ads_client_mock_(std::make_unique<NiceMock<AdsClientMock>>()),
        ads_(std::make_unique<AdsImpl>(ads_client_mock_.get())),
        locale_helper_mock_(std::make_unique<
            NiceMock<brave_l10n::LocaleHelperMock>>()),
        platform_helper_mock_(std::make_unique<
            NiceMock<PlatformHelperMock>>()),
        database_table_(std::make_unique<
            database::table::AdConversions>(ads_.get())) {
    // You can do set-up work for each test here

    brave_l10n::LocaleHelper::GetInstance()->set_for_testing(
        locale_helper_mock_.get());

    PlatformHelper::GetInstance()->set_for_testing(platform_helper_mock_.get());
  }

  ~BatAdsAdConversionsDatabaseTableTest() override {
    // You can do clean-up work that doesn't throw exceptions here
  }

  // If the constructor and destructor are not enough for setting up and
  // cleaning up each test, you can use the following methods

  void SetUp() override {
    // Code here will be called immediately after the constructor (right before
    // each test)

    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    const base::FilePath path = temp_dir_.GetPath();

    database_ = std::make_unique<Database>(path.AppendASCII("database.sqlite"));
    MockRunDBTransaction(ads_client_mock_, database_);
  }

  void TearDown() override {
    // Code here will be called immediately after each test (right before the
    // destructor)
  }

  // Objects declared here can be used by all tests in the test case

  void CreateOrOpenDatabase() {
    database::Initialize initialize(ads_.get());
    initialize.CreateOrOpen([](
        const Result result) {
      ASSERT_EQ(Result::SUCCESS, result);
    });
  }

  void SaveDatabase(
      const AdConversionList ad_conversions) {
    database_table_->Save(ad_conversions, [](
        const Result result) {
      ASSERT_EQ(Result::SUCCESS, result);
    });
  }

  void PurgeExpiredAdConversions() {
    database_table_->PurgeExpiredAdConversions([](
        const Result result) {
      ASSERT_EQ(Result::SUCCESS, result);
    });
  }

  int64_t CalculateExpiryTimestamp(
      const int observation_window) {
    base::Time time = base::Time::Now();
    time += base::TimeDelta::FromDays(observation_window);
    return static_cast<int64_t>(time.ToDoubleT());
  }

  base::test::TaskEnvironment task_environment_;

  base::ScopedTempDir temp_dir_;

  std::unique_ptr<AdsClientMock> ads_client_mock_;
  std::unique_ptr<AdsImpl> ads_;
  std::unique_ptr<brave_l10n::LocaleHelperMock> locale_helper_mock_;
  std::unique_ptr<PlatformHelperMock> platform_helper_mock_;
  std::unique_ptr<database::table::AdConversions> database_table_;
  std::unique_ptr<Database> database_;
};

TEST_F(BatAdsAdConversionsDatabaseTableTest,
    EmptySave) {
  // Arrange
  CreateOrOpenDatabase();

  AdConversionList ad_conversions = {};

  // Act
  SaveDatabase(ad_conversions);

  // Assert
  const AdConversionList expected_ad_conversions = ad_conversions;

  database_table_->GetAdConversions([&expected_ad_conversions](
      const Result result,
      const AdConversionList& ad_conversions) {
    EXPECT_EQ(Result::SUCCESS, result);
    EXPECT_TRUE(CompareAsSets(expected_ad_conversions, ad_conversions));
  });
}

TEST_F(BatAdsAdConversionsDatabaseTableTest,
    SaveAdConversions) {
  // Arrange
  CreateOrOpenDatabase();

  AdConversionList ad_conversions;

  AdConversionInfo info_1;
  info_1.creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  info_1.type = "postview";
  info_1.url_pattern = "https://www.brave.com/*";
  info_1.observation_window = 3;
  info_1.expiry_timestamp = CalculateExpiryTimestamp(info_1.observation_window);
  ad_conversions.push_back(info_1);

  AdConversionInfo info_2;
  info_2.creative_set_id = "eaa6224a-46a4-4c48-9c2b-c264c0067f04";
  info_2.type = "postclick";
  info_2.url_pattern = "https://www.brave.com/signup/*";
  info_2.observation_window = 30;
  info_2.expiry_timestamp = CalculateExpiryTimestamp(info_2.observation_window);
  ad_conversions.push_back(info_2);

  // Act
  SaveDatabase(ad_conversions);

  // Assert
  const AdConversionList expected_ad_conversions = ad_conversions;

  database_table_->GetAdConversions([&expected_ad_conversions](
      const Result result,
      const AdConversionList& ad_conversions) {
    EXPECT_EQ(Result::SUCCESS, result);
    EXPECT_TRUE(CompareAsSets(expected_ad_conversions, ad_conversions));
  });
}

TEST_F(BatAdsAdConversionsDatabaseTableTest,
    DoNotSaveDuplicateAdConversion) {
  // Arrange
  CreateOrOpenDatabase();

  AdConversionList ad_conversions;

  AdConversionInfo info;
  info.creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  info.type = "postview";
  info.url_pattern = "https://www.brave.com/*";
  info.observation_window = 3;
  info.expiry_timestamp = CalculateExpiryTimestamp(info.observation_window);
  ad_conversions.push_back(info);

  SaveDatabase(ad_conversions);

  // Act
  SaveDatabase(ad_conversions);

  // Assert
  const AdConversionList expected_ad_conversions = ad_conversions;

  database_table_->GetAdConversions([&expected_ad_conversions](
      const Result result,
      const AdConversionList& ad_conversions) {
    EXPECT_EQ(Result::SUCCESS, result);
    EXPECT_TRUE(CompareAsSets(expected_ad_conversions, ad_conversions));
  });
}

TEST_F(BatAdsAdConversionsDatabaseTableTest,
    PurgeExpiredAdConversions) {
  // Arrange
  CreateOrOpenDatabase();

  AdConversionList ad_conversions;

  AdConversionInfo info_1;
  info_1.creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  info_1.type = "postview";
  info_1.url_pattern = "https://www.brave.com/*";
  info_1.observation_window = 7;
  info_1.expiry_timestamp = CalculateExpiryTimestamp(info_1.observation_window);
  ad_conversions.push_back(info_1);

  AdConversionInfo info_2;  // Should be purged
  info_2.creative_set_id = "eaa6224a-46a4-4c48-9c2b-c264c0067f04";
  info_2.type = "postclick";
  info_2.url_pattern = "https://www.brave.com/signup/*";
  info_2.observation_window = 3;
  info_2.expiry_timestamp = CalculateExpiryTimestamp(info_2.observation_window);
  ad_conversions.push_back(info_2);

  AdConversionInfo info_3;
  info_3.creative_set_id = "8e9f0c2f-1640-463c-902d-ca711789287f";
  info_3.type = "postview";
  info_3.url_pattern = "https://www.brave.com/*";
  info_3.observation_window = 30;
  info_3.expiry_timestamp = CalculateExpiryTimestamp(info_3.observation_window);
  ad_conversions.push_back(info_3);

  SaveDatabase(ad_conversions);

  // Act
  task_environment_.FastForwardBy(base::TimeDelta::FromDays(4));

  PurgeExpiredAdConversions();

  // Assert
  AdConversionList expected_ad_conversions;
  expected_ad_conversions.push_back(info_1);
  expected_ad_conversions.push_back(info_3);

  database_table_->GetAdConversions([&expected_ad_conversions](
      const Result result,
      const AdConversionList& ad_conversions) {
    EXPECT_EQ(Result::SUCCESS, result);
    EXPECT_TRUE(CompareAsSets(expected_ad_conversions, ad_conversions));
  });
}

TEST_F(BatAdsAdConversionsDatabaseTableTest,
    SaveAdConversionWithMatchingCreativeSetIdAndTypeAndUrlPattern) {
  // Arrange
  CreateOrOpenDatabase();

  AdConversionList ad_conversions;

  AdConversionInfo info_1;
  info_1.creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  info_1.type = "postview";
  info_1.url_pattern = "https://www.brave.com/*";
  info_1.observation_window = 3;
  info_1.expiry_timestamp = CalculateExpiryTimestamp(info_1.observation_window);
  ad_conversions.push_back(info_1);

  SaveDatabase(ad_conversions);

  // Act
  AdConversionInfo info_2;  // Should supersede info_1
  info_2.creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  info_2.type = "postview";
  info_2.url_pattern = "https://www.brave.com/*";
  info_2.observation_window = 30;
  info_2.expiry_timestamp = CalculateExpiryTimestamp(info_2.observation_window);
  ad_conversions.push_back(info_2);

  SaveDatabase(ad_conversions);

  // Assert
  AdConversionList expected_ad_conversions;
  expected_ad_conversions.push_back(info_2);

  database_table_->GetAdConversions([&expected_ad_conversions](
      const Result result,
      const AdConversionList& ad_conversions) {
    EXPECT_EQ(Result::SUCCESS, result);
    EXPECT_TRUE(CompareAsSets(expected_ad_conversions, ad_conversions));
  });
}

TEST_F(BatAdsAdConversionsDatabaseTableTest,
    GetAdConversionsFromCatalogEndpoint) {
  // Arrange
  ON_CALL(*ads_client_mock_, IsEnabled())
      .WillByDefault(Return(true));

  ON_CALL(*ads_client_mock_, ShouldAllowAdConversionTracking())
      .WillByDefault(Return(true));

  SetBuildChannel(false, "test");

  ON_CALL(*locale_helper_mock_, GetLocale())
      .WillByDefault(Return("en-US"));

  MockPlatformHelper(platform_helper_mock_, PlatformType::kMacOS);

  ads_->OnWalletUpdated("c387c2d8-a26d-4451-83e4-5c0c6fd942be",
      "5BEKM1Y7xcRSg/1q8in/+Lki2weFZQB+UMYZlRw8ql8=");

  MockLoad(ads_client_mock_);
  MockLoadUserModelForId(ads_client_mock_);
  MockLoadResourceForId(ads_client_mock_);
  MockSave(ads_client_mock_);

  const URLEndpoints endpoints = {
    {
      "/v3/catalog", {
        {
          net::HTTP_OK, "/catalog.json"
        }
      }
    }
  };

  MockUrlRequest(ads_client_mock_, endpoints);

  Initialize(ads_);

  // Act

  // Assert
  database_table_->GetAdConversions([](
      const Result result,
      const AdConversionList& ad_conversions) {
    EXPECT_EQ(Result::SUCCESS, result);
    EXPECT_EQ(2UL, ad_conversions.size());
  });
}

TEST_F(BatAdsAdConversionsDatabaseTableTest,
    TableName) {
  // Arrange

  // Act
  const std::string table_name = database_table_->get_table_name();

  // Assert
  const std::string expected_table_name = "ad_conversions";
  EXPECT_EQ(expected_table_name, table_name);
}

}  // namespace ads
