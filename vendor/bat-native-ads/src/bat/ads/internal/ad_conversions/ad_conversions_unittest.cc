/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_conversions/ad_conversions.h"

#include <stdint.h>

#include <deque>
#include <map>
#include <memory>
#include <string>
#include <utility>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/test/task_environment.h"
#include "brave/components/l10n/browser/locale_helper_mock.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "bat/ads/internal/ads_client_mock.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/database/tables/ad_conversions_database_table.h"
#include "bat/ads/internal/platform/platform_helper_mock.h"
#include "bat/ads/internal/time_util.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

using ::testing::NiceMock;
using ::testing::Return;

namespace ads {

class BatAdsAdConversionsTest : public ::testing::Test {
 protected:
  BatAdsAdConversionsTest()
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

  ~BatAdsAdConversionsTest() override {
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

    database_ = std::make_unique<Database>(path.AppendASCII("database.sqlite"));
    MockRunDBTransaction(ads_client_mock_, database_);

    Initialize(ads_);
  }

  void TearDown() override {
    // Code here will be called immediately after each test (right before the
    // destructor)
  }

  // Objects declared here can be used by all tests in the test case

  Client* get_client() {
    return ads_->get_client();
  }

  AdConversions* get_ad_conversions() {
    return ads_->get_ad_conversions();
  }

  std::deque<uint64_t> GetAdConversionHistoryForCreativeSet(
      const std::string& creative_set_id) {
    std::deque<uint64_t> creative_set_history;

    const std::map<std::string, std::deque<uint64_t>> history =
        get_client()->GetAdConversionHistory();

    const auto iter = history.find(creative_set_id);
    if (iter == history.end()) {
      return creative_set_history;
    }

    creative_set_history = iter->second;
    return creative_set_history;
  }

  void SaveAdConversions(
      const AdConversionList& ad_conversions) {
    database_table_->Save(ad_conversions, [](
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

  void TriggerAdEvent(
      const std::string& creative_set_id,
      const ConfirmationType confirmation_type) {
    AdHistory history;

    history.ad_content.creative_instance_id =
        "7a3b6d9f-d0b7-4da6-8988-8d5b8938c94f";
    history.ad_content.creative_set_id = creative_set_id;
    history.ad_content.ad_action = confirmation_type;
    history.timestamp_in_seconds = base::Time::Now().ToDoubleT();

    get_client()->AppendAdHistoryToAdsHistory(history);
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

TEST_F(BatAdsAdConversionsTest,
    ShouldNotAllowAdConversionTracking) {
  // Arrange
  AdConversionList ad_conversions;

  AdConversionInfo info;
  info.creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  info.type = "postview";
  info.url_pattern = "https://www.brave.com/*";
  info.observation_window = 3;
  info.expiry_timestamp = CalculateExpiryTimestamp(info.observation_window);
  ad_conversions.push_back(info);

  SaveAdConversions(ad_conversions);

  ON_CALL(*ads_client_mock_, ShouldAllowAdConversionTracking())
      .WillByDefault(Return(false));

  // Act
  get_ad_conversions()->MaybeConvert("https://www.brave.com/signup");

  // Assert
  const std::deque<uint64_t> creative_set_history =
      GetAdConversionHistoryForCreativeSet(info.creative_set_id);

  EXPECT_TRUE(creative_set_history.empty());
}

TEST_F(BatAdsAdConversionsTest,
    ConvertViewedAd) {
  // Arrange
  AdConversionList ad_conversions;

  AdConversionInfo info;
  info.creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  info.type = "postview";
  info.url_pattern = "https://www.brave.com/*";
  info.observation_window = 3;
  info.expiry_timestamp = CalculateExpiryTimestamp(info.observation_window);
  ad_conversions.push_back(info);

  SaveAdConversions(ad_conversions);

  TriggerAdEvent(info.creative_set_id, ConfirmationType::kViewed);

  // Act
  get_ad_conversions()->MaybeConvert("https://www.brave.com/signup");

  // Assert
  const std::deque<uint64_t> creative_set_history =
      GetAdConversionHistoryForCreativeSet(info.creative_set_id);

  EXPECT_EQ(1UL, creative_set_history.size());
}


TEST_F(BatAdsAdConversionsTest,
    DoNotConvertAdIfConversionDoesNotExist) {
  // Arrange
  const std::string creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";

  TriggerAdEvent(creative_set_id, ConfirmationType::kViewed);

  // Act
  get_ad_conversions()->MaybeConvert("https://www.brave.com/signup");

  // Assert
  const std::deque<uint64_t> creative_set_history =
      GetAdConversionHistoryForCreativeSet(creative_set_id);

  EXPECT_EQ(0UL, creative_set_history.size());
}

TEST_F(BatAdsAdConversionsTest,
    ConvertClickedAd) {
  // Arrange
  AdConversionList ad_conversions;

  AdConversionInfo info;
  info.creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  info.type = "postclick";
  info.url_pattern = "https://www.brave.com/*";
  info.observation_window = 3;
  info.expiry_timestamp = CalculateExpiryTimestamp(info.observation_window);
  ad_conversions.push_back(info);

  SaveAdConversions(ad_conversions);

  TriggerAdEvent(info.creative_set_id, ConfirmationType::kClicked);

  // Act
  get_ad_conversions()->MaybeConvert("https://www.brave.com/signup");

  // Assert
  const std::deque<uint64_t> creative_set_history =
      GetAdConversionHistoryForCreativeSet(info.creative_set_id);

  EXPECT_EQ(1UL, creative_set_history.size());
}

TEST_F(BatAdsAdConversionsTest,
    DoNotConvertNonViewedOrClickedAds) {
  // Arrange
  AdConversionList ad_conversions;

  AdConversionInfo info;
  info.creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  info.type = "postclick";
  info.url_pattern = "https://www.brave.com/*";
  info.observation_window = 3;
  info.expiry_timestamp = CalculateExpiryTimestamp(info.observation_window);
  ad_conversions.push_back(info);

  SaveAdConversions(ad_conversions);

  TriggerAdEvent(info.creative_set_id, ConfirmationType::kNone);
  TriggerAdEvent(info.creative_set_id, ConfirmationType::kDismissed);
  TriggerAdEvent(info.creative_set_id, ConfirmationType::kLanded);
  TriggerAdEvent(info.creative_set_id, ConfirmationType::kFlagged);
  TriggerAdEvent(info.creative_set_id, ConfirmationType::kUpvoted);
  TriggerAdEvent(info.creative_set_id, ConfirmationType::kDownvoted);
  TriggerAdEvent(info.creative_set_id, ConfirmationType::kConversion);

  // Act
  get_ad_conversions()->MaybeConvert("https://www.brave.com/signup");

  // Assert
  const std::deque<uint64_t> creative_set_history =
      GetAdConversionHistoryForCreativeSet(info.creative_set_id);

  EXPECT_TRUE(creative_set_history.empty());
}

TEST_F(BatAdsAdConversionsTest,
    DoNotConvertAdWhenThereIsAdConversionHistoryForTheSameCreativeSet) {
  // Arrange
  AdConversionList ad_conversions;

  AdConversionInfo info;
  info.creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  info.type = "postview";
  info.url_pattern = "https://www.brave.com/*";
  info.observation_window = 3;
  info.expiry_timestamp = CalculateExpiryTimestamp(info.observation_window);
  ad_conversions.push_back(info);

  SaveAdConversions(ad_conversions);

  TriggerAdEvent(info.creative_set_id, ConfirmationType::kViewed);

  get_ad_conversions()->MaybeConvert("https://www.brave.com/signup");

  // Act
  get_ad_conversions()->MaybeConvert("https://www.brave.com/signup");

  // Assert
  const std::deque<uint64_t> creative_set_history =
      GetAdConversionHistoryForCreativeSet(info.creative_set_id);

  EXPECT_EQ(1UL, creative_set_history.size());
}

TEST_F(BatAdsAdConversionsTest,
    DoNotConvertAdWhenUrlDoesNotMatchAdConversionPattern) {
  // Arrange
  AdConversionList ad_conversions;

  AdConversionInfo info;
  info.creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  info.type = "postview";
  info.url_pattern = "https://www.brave.com/signup/*";
  info.observation_window = 3;
  info.expiry_timestamp = CalculateExpiryTimestamp(info.observation_window);
  ad_conversions.push_back(info);

  SaveAdConversions(ad_conversions);

  TriggerAdEvent(info.creative_set_id, ConfirmationType::kViewed);

  // Act
  get_ad_conversions()->MaybeConvert("https://www.brave.com/welcome");

  // Assert
  const std::deque<uint64_t> creative_set_history =
      GetAdConversionHistoryForCreativeSet(info.creative_set_id);

  EXPECT_TRUE(creative_set_history.empty());
}

TEST_F(BatAdsAdConversionsTest,
    ConvertAdWhenTheAdConversionObservationWindowHasNotExpired) {
  // Arrange
  AdConversionList ad_conversions;

  AdConversionInfo info;
  info.creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  info.type = "postview";
  info.url_pattern = "https://www.brave.com/*";
  info.observation_window = 3;
  info.expiry_timestamp = CalculateExpiryTimestamp(info.observation_window);
  ad_conversions.push_back(info);

  SaveAdConversions(ad_conversions);

  TriggerAdEvent(info.creative_set_id, ConfirmationType::kViewed);

  task_environment_.FastForwardBy(base::TimeDelta::FromDays(2));

  // Act
  get_ad_conversions()->MaybeConvert("https://www.brave.com/");

  // Assert
  const std::deque<uint64_t> creative_set_history =
      GetAdConversionHistoryForCreativeSet(info.creative_set_id);

  EXPECT_EQ(1UL, creative_set_history.size());
}

TEST_F(BatAdsAdConversionsTest,
    DoNotConvertAdWhenTheAdConversionObservationWindowIsOnTheCuspOfExpiring) {
  // Arrange
  AdConversionList ad_conversions;

  AdConversionInfo info;
  info.creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  info.type = "postview";
  info.url_pattern = "https://www.brave.com/signup/*";
  info.observation_window = 3;
  info.expiry_timestamp = CalculateExpiryTimestamp(info.observation_window);
  ad_conversions.push_back(info);

  SaveAdConversions(ad_conversions);

  TriggerAdEvent(info.creative_set_id, ConfirmationType::kViewed);

  task_environment_.FastForwardBy(base::TimeDelta::FromHours(71));

  // Act
  get_ad_conversions()->MaybeConvert("https://www.foobar.com/");

  // Assert
  const std::deque<uint64_t> creative_set_history =
      GetAdConversionHistoryForCreativeSet(info.creative_set_id);

  EXPECT_TRUE(creative_set_history.empty());
}

TEST_F(BatAdsAdConversionsTest,
    DoNotConvertAdWhenTheAdConversionObservationWindowHasExpired) {
  // Arrange
  AdConversionList ad_conversions;

  AdConversionInfo info;
  info.creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  info.type = "postview";
  info.url_pattern = "https://www.brave.com/signup/*";
  info.observation_window = 3;
  info.expiry_timestamp = CalculateExpiryTimestamp(info.observation_window);
  ad_conversions.push_back(info);

  SaveAdConversions(ad_conversions);

  TriggerAdEvent(info.creative_set_id, ConfirmationType::kViewed);

  task_environment_.FastForwardBy(base::TimeDelta::FromDays(4));

  // Act
  get_ad_conversions()->MaybeConvert("https://www.foobar.com/");

  // Assert
  const std::deque<uint64_t> creative_set_history =
      GetAdConversionHistoryForCreativeSet(info.creative_set_id);

  EXPECT_TRUE(creative_set_history.empty());
}

TEST_F(BatAdsAdConversionsTest,
    ConvertAdWhenTheAdConversionIsOnTheCuspOfExpiring) {
  // Arrange
  AdConversionList ad_conversions;

  AdConversionInfo info;
  info.creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  info.type = "postview";
  info.url_pattern = "https://www.brave.com/*";
  info.observation_window = 3;
  info.expiry_timestamp = CalculateExpiryTimestamp(info.observation_window);
  ad_conversions.push_back(info);

  SaveAdConversions(ad_conversions);

  TriggerAdEvent(info.creative_set_id, ConfirmationType::kViewed);

  task_environment_.FastForwardBy(base::TimeDelta::FromHours(71));

  // Act
  get_ad_conversions()->MaybeConvert("https://www.brave.com/");

  // Assert
  const std::deque<uint64_t> creative_set_history =
      GetAdConversionHistoryForCreativeSet(info.creative_set_id);

  EXPECT_EQ(1UL, creative_set_history.size());
}

TEST_F(BatAdsAdConversionsTest,
    DoNotConvertAdWhenTheAdConversionHasExpired) {
  // Arrange
  AdConversionList ad_conversions;

  AdConversionInfo info;
  info.creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  info.type = "postview";
  info.url_pattern = "https://www.brave.com/signup/*";
  info.observation_window = 3;
  info.expiry_timestamp = CalculateExpiryTimestamp(info.observation_window);
  ad_conversions.push_back(info);

  SaveAdConversions(ad_conversions);

  TriggerAdEvent(info.creative_set_id, ConfirmationType::kViewed);

  task_environment_.FastForwardBy(base::TimeDelta::FromDays(4));

  // Act
  get_ad_conversions()->MaybeConvert("https://www.brave.com/");

  // Assert
  const std::deque<uint64_t> creative_set_history =
      GetAdConversionHistoryForCreativeSet(info.creative_set_id);

  EXPECT_TRUE(creative_set_history.empty());
}

}  // namespace ads
