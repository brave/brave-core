/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/conversions/conversions.h"

#include <stdint.h>

#include <deque>
#include <map>
#include <memory>
#include <string>
#include <utility>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/strings/stringprintf.h"
#include "base/test/task_environment.h"
#include "brave/components/l10n/browser/locale_helper_mock.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "bat/ads/internal/ad_events/ad_events.h"
#include "bat/ads/internal/ads_client_mock.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/client/client.h"
#include "bat/ads/internal/database/tables/ad_events_database_table.h"
#include "bat/ads/internal/database/tables/conversions_database_table.h"
#include "bat/ads/internal/platform/platform_helper_mock.h"
#include "bat/ads/internal/time_util.h"
#include "bat/ads/internal/unittest_util.h"
#include "bat/ads/pref_names.h"

// npm run test -- brave_unit_tests --filter=BatAds*

using ::testing::NiceMock;
using ::testing::Return;

namespace ads {

class BatAdsConversionsTest : public ::testing::Test {
 protected:
  BatAdsConversionsTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME),
        ads_client_mock_(std::make_unique<NiceMock<AdsClientMock>>()),
        ads_(std::make_unique<AdsImpl>(ads_client_mock_.get())),
        locale_helper_mock_(std::make_unique<
            NiceMock<brave_l10n::LocaleHelperMock>>()),
        platform_helper_mock_(std::make_unique<
            NiceMock<PlatformHelperMock>>()),
        conversions_database_table_(std::make_unique<
            database::table::Conversions>(ads_.get())),
        ad_events_database_table_(std::make_unique<
            database::table::AdEvents>(ads_.get())) {
    // You can do set-up work for each test here

    brave_l10n::LocaleHelper::GetInstance()->set_for_testing(
        locale_helper_mock_.get());

    PlatformHelper::GetInstance()->set_for_testing(platform_helper_mock_.get());
  }

  ~BatAdsConversionsTest() override {
    // You can do clean-up work that doesn't throw exceptions here
  }

  // If the constructor and destructor are not enough for setting up and
  // cleaning up each test, you can use the following methods

  void SetUp() override {
    // Code here will be called immediately after the constructor (right before
    // each test)

    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    const base::FilePath path = temp_dir_.GetPath();

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

    MockPrefs(ads_client_mock_);

    database_ = std::make_unique<Database>(path.AppendASCII("database.sqlite"));
    MockRunDBTransaction(ads_client_mock_, database_);

    Initialize(ads_);
  }

  void TearDown() override {
    // Code here will be called immediately after each test (right before the
    // destructor)
  }

  // Objects declared here can be used by all tests in the test case

  void SaveConversions(
      const ConversionList& conversions) {
    conversions_database_table_->Save(conversions, [](
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
    AdEventInfo ad_event;
    ad_event.creative_instance_id = "7a3b6d9f-d0b7-4da6-8988-8d5b8938c94f";
    ad_event.creative_set_id = creative_set_id;
    ad_event.timestamp = base::Time::Now().ToDoubleT();
    ad_event.confirmation_type = confirmation_type;

    AdEvents ad_events(ads_.get());
    ad_events.Log(ad_event,
        [](const Result result) {
      ASSERT_EQ(Result::SUCCESS, result);
    });
  }

  base::test::TaskEnvironment task_environment_;

  base::ScopedTempDir temp_dir_;

  std::unique_ptr<AdsClientMock> ads_client_mock_;
  std::unique_ptr<AdsImpl> ads_;
  std::unique_ptr<brave_l10n::LocaleHelperMock> locale_helper_mock_;
  std::unique_ptr<PlatformHelperMock> platform_helper_mock_;
  std::unique_ptr<database::table::Conversions> conversions_database_table_;
  std::unique_ptr<database::table::AdEvents> ad_events_database_table_;
  std::unique_ptr<Database> database_;
};

TEST_F(BatAdsConversionsTest,
    ShouldNotAllowConversionTracking) {
  // Arrange
  ads_client_mock_->SetBooleanPref(
      ads::prefs::kShouldAllowConversionTracking, false);

  ConversionList conversions;

  ConversionInfo conversion;
  conversion.creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  conversion.type = "postview";
  conversion.url_pattern = "https://www.foobar.com/*";
  conversion.observation_window = 3;
  conversion.expiry_timestamp =
      CalculateExpiryTimestamp(conversion.observation_window);
  conversions.push_back(conversion);

  SaveConversions(conversions);

  // Act
  ads_->get_conversions()->MaybeConvert("https://www.foobar.com/signup");

  // Assert
  const std::string condition = base::StringPrintf(
      "creative_set_id = '%s' AND confirmation_type = 'conversion'",
          conversion.creative_set_id.c_str());

  ad_events_database_table_->GetIf(condition, [](
      const Result result,
      const AdEventList& ad_events) {
    ASSERT_EQ(Result::SUCCESS, result);

    EXPECT_TRUE(ad_events.empty());
  });
}

TEST_F(BatAdsConversionsTest,
    ConvertViewedAd) {
  // Arrange
  ConversionList conversions;

  ConversionInfo conversion;
  conversion.creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  conversion.type = "postview";
  conversion.url_pattern = "https://www.foo.com/*";
  conversion.observation_window = 3;
  conversion.expiry_timestamp =
      CalculateExpiryTimestamp(conversion.observation_window);
  conversions.push_back(conversion);

  SaveConversions(conversions);

  TriggerAdEvent(conversion.creative_set_id, ConfirmationType::kViewed);

  // Act
  ads_->get_conversions()->MaybeConvert("https://www.foo.com/bar");

  // Assert
  const std::string condition = base::StringPrintf(
      "creative_set_id = '%s' AND confirmation_type = 'conversion'",
          conversion.creative_set_id.c_str());

  ad_events_database_table_->GetIf(condition, [&conversion](
      const Result result,
      const AdEventList& ad_events) {
    ASSERT_EQ(Result::SUCCESS, result);

    EXPECT_EQ(1UL, ad_events.size());
    AdEventInfo ad_event = ad_events.front();

    EXPECT_EQ(conversion.creative_set_id, ad_event.creative_set_id);
  });
}

TEST_F(BatAdsConversionsTest,
    ConvertClickedAd) {
  // Arrange
  ConversionList conversions;

  ConversionInfo conversion;
  conversion.creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  conversion.type = "postclick";
  conversion.url_pattern = "https://www.foo.com/*/baz";
  conversion.observation_window = 3;
  conversion.expiry_timestamp =
      CalculateExpiryTimestamp(conversion.observation_window);
  conversions.push_back(conversion);

  SaveConversions(conversions);

  TriggerAdEvent(conversion.creative_set_id, ConfirmationType::kViewed);
  TriggerAdEvent(conversion.creative_set_id, ConfirmationType::kClicked);

  // Act
  ads_->get_conversions()->MaybeConvert("https://www.foo.com/bar/baz");

  // Assert
  const std::string condition = base::StringPrintf(
      "creative_set_id = '%s' AND confirmation_type = 'conversion'",
          conversion.creative_set_id.c_str());

  ad_events_database_table_->GetIf(condition, [&conversion](
      const Result result,
      const AdEventList& ad_events) {
    ASSERT_EQ(Result::SUCCESS, result);

    EXPECT_EQ(1UL, ad_events.size());
    AdEventInfo ad_event = ad_events.front();

    EXPECT_EQ(conversion.creative_set_id, ad_event.creative_set_id);
  });
}

TEST_F(BatAdsConversionsTest,
    ConvertMultipleAds) {
  // Arrange
  ConversionList conversions;

  ConversionInfo conversion_1;
  conversion_1.creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  conversion_1.type = "postview";
  conversion_1.url_pattern = "https://www.foo.com/*";
  conversion_1.observation_window = 3;
  conversion_1.expiry_timestamp =
      CalculateExpiryTimestamp(conversion_1.observation_window);
  conversions.push_back(conversion_1);

  ConversionInfo conversion_2;
  conversion_2.creative_set_id = "4e83a23c-1194-40f8-8fdc-2f38d7ed75c8";
  conversion_2.type = "postclick";
  conversion_2.url_pattern = "https://www.foo.com/*/baz";
  conversion_2.observation_window = 3;
  conversion_2.expiry_timestamp =
      CalculateExpiryTimestamp(conversion_2.observation_window);
  conversions.push_back(conversion_2);

  SaveConversions(conversions);

  TriggerAdEvent(conversion_1.creative_set_id, ConfirmationType::kViewed);

  TriggerAdEvent(conversion_2.creative_set_id, ConfirmationType::kViewed);
  TriggerAdEvent(conversion_2.creative_set_id, ConfirmationType::kClicked);

  // Act
  ads_->get_conversions()->MaybeConvert("https://www.foo.com/qux");

  ads_->get_conversions()->MaybeConvert("https://www.foo.com/bar/baz");

  // Assert
  const std::string condition = base::StringPrintf(
      "(creative_set_id = '%s' OR creative_set_id = '%s') AND "
          "confirmation_type = 'conversion'",
      conversion_1.creative_set_id.c_str(),
      conversion_2.creative_set_id.c_str());

  ad_events_database_table_->GetIf(condition, [&conversions](
      const Result result,
      const AdEventList& ad_events) {
    ASSERT_EQ(Result::SUCCESS, result);

    EXPECT_EQ(2UL, ad_events.size());

    const ConversionInfo conversion_1 = conversions.at(0);
    const AdEventInfo ad_event_1 = ad_events.at(0);
    EXPECT_EQ(conversion_1.creative_set_id, ad_event_1.creative_set_id);

    const ConversionInfo conversion_2 = conversions.at(1);
    const AdEventInfo ad_event_2 = ad_events.at(1);
    EXPECT_EQ(conversion_2.creative_set_id, ad_event_2.creative_set_id);
  });
}

TEST_F(BatAdsConversionsTest,
    ConvertViewedAdWhenAdWasDismissed) {
  // Arrange
  ConversionList conversions;

  ConversionInfo conversion;
  conversion.creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  conversion.type = "postview";
  conversion.url_pattern = "https://www.foo.com/*bar*";
  conversion.observation_window = 3;
  conversion.expiry_timestamp =
      CalculateExpiryTimestamp(conversion.observation_window);
  conversions.push_back(conversion);

  SaveConversions(conversions);

  TriggerAdEvent(conversion.creative_set_id, ConfirmationType::kViewed);
  TriggerAdEvent(conversion.creative_set_id, ConfirmationType::kDismissed);

  // Act
  ads_->get_conversions()->MaybeConvert("https://www.foo.com/quxbarbaz");

  // Assert
  const std::string condition = base::StringPrintf(
      "creative_set_id = '%s' AND confirmation_type = 'conversion'",
          conversion.creative_set_id.c_str());

  ad_events_database_table_->GetIf(condition, [&conversion](
      const Result result,
      const AdEventList& ad_events) {
    ASSERT_EQ(Result::SUCCESS, result);

    EXPECT_EQ(1UL, ad_events.size());
    AdEventInfo ad_event = ad_events.front();

    EXPECT_EQ(conversion.creative_set_id, ad_event.creative_set_id);
  });
}

TEST_F(BatAdsConversionsTest,
    DoNotConvertNonViewedOrClickedAds) {
  // Arrange
  ConversionList conversions;

  ConversionInfo conversion;
  conversion.creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  conversion.type = "postclick";
  conversion.url_pattern = "https://www.foo.com/bar";
  conversion.observation_window = 3;
  conversion.expiry_timestamp =
      CalculateExpiryTimestamp(conversion.observation_window);
  conversions.push_back(conversion);

  SaveConversions(conversions);

  TriggerAdEvent(conversion.creative_set_id, ConfirmationType::kDismissed);
  TriggerAdEvent(conversion.creative_set_id, ConfirmationType::kTransferred);
  TriggerAdEvent(conversion.creative_set_id, ConfirmationType::kFlagged);
  TriggerAdEvent(conversion.creative_set_id, ConfirmationType::kUpvoted);
  TriggerAdEvent(conversion.creative_set_id, ConfirmationType::kDownvoted);

  // Act
  ads_->get_conversions()->MaybeConvert("https://www.foo.com/bar");

  // Assert
  const std::string condition = base::StringPrintf(
      "creative_set_id = '%s' AND confirmation_type = 'conversion'",
          conversion.creative_set_id.c_str());

  ad_events_database_table_->GetIf(condition, [](
      const Result result,
      const AdEventList& ad_events) {
    ASSERT_EQ(Result::SUCCESS, result);

    EXPECT_TRUE(ad_events.empty());
  });
}

TEST_F(BatAdsConversionsTest,
    DoNotConvertAdIfConversionDoesNotExist) {
  // Arrange
  const std::string creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";

  TriggerAdEvent(creative_set_id, ConfirmationType::kViewed);

  // Act
  ads_->get_conversions()->MaybeConvert("https://www.foo.com/bar");

  // Assert
  const std::string condition = "creative_set_id = 'foobar' AND "
      "confirmation_type = 'conversion'";

  ad_events_database_table_->GetIf(condition, [](
      const Result result,
      const AdEventList& ad_events) {
    ASSERT_EQ(Result::SUCCESS, result);

    EXPECT_TRUE(ad_events.empty());
  });
}

TEST_F(BatAdsConversionsTest,
    DoNotConvertAdWhenThereIsConversionHistoryForTheSameCreativeSet) {
  // Arrange
  ConversionList conversions;

  ConversionInfo conversion;
  conversion.creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  conversion.type = "postview";
  conversion.url_pattern = "https://www.foo.com/*";
  conversion.observation_window = 3;
  conversion.expiry_timestamp =
      CalculateExpiryTimestamp(conversion.observation_window);
  conversions.push_back(conversion);

  SaveConversions(conversions);

  TriggerAdEvent(conversion.creative_set_id, ConfirmationType::kViewed);

  ads_->get_conversions()->MaybeConvert("https://www.foo.com/bar");

  // Act
  ads_->get_conversions()->MaybeConvert("https://www.foo.com/bar");

  // Assert
  const std::string condition = base::StringPrintf(
      "creative_set_id = '%s' AND confirmation_type = 'conversion'",
          conversion.creative_set_id.c_str());

  ad_events_database_table_->GetIf(condition, [&conversion](
      const Result result,
      const AdEventList& ad_events) {
    ASSERT_EQ(Result::SUCCESS, result);

    EXPECT_EQ(1UL, ad_events.size());
    AdEventInfo ad_event = ad_events.front();

    EXPECT_EQ(conversion.creative_set_id, ad_event.creative_set_id);
  });
}

TEST_F(BatAdsConversionsTest,
    DoNotConvertAdWhenUrlDoesNotMatchConversionPattern) {
  // Arrange
  ConversionList conversions;

  ConversionInfo conversion;
  conversion.creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  conversion.type = "postview";
  conversion.url_pattern = "https://www.foo.com/bar/*";
  conversion.observation_window = 3;
  conversion.expiry_timestamp =
      CalculateExpiryTimestamp(conversion.observation_window);
  conversions.push_back(conversion);

  SaveConversions(conversions);

  TriggerAdEvent(conversion.creative_set_id, ConfirmationType::kViewed);

  // Act
  ads_->get_conversions()->MaybeConvert("https://www.foo.com/qux");

  // Assert
  const std::string condition = base::StringPrintf(
      "creative_set_id = '%s' AND confirmation_type = 'conversion'",
          conversion.creative_set_id.c_str());

  ad_events_database_table_->GetIf(condition, [](
      const Result result,
      const AdEventList& ad_events) {
    ASSERT_EQ(Result::SUCCESS, result);

    EXPECT_TRUE(ad_events.empty());
  });
}

TEST_F(BatAdsConversionsTest,
    ConvertAdWhenTheConversionIsOnTheCuspOfExpiring) {
  // Arrange
  ConversionList conversions;

  ConversionInfo conversion;
  conversion.creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  conversion.type = "postview";
  conversion.url_pattern = "https://*.bar.com/*";
  conversion.observation_window = 3;
  conversion.expiry_timestamp =
      CalculateExpiryTimestamp(conversion.observation_window);
  conversions.push_back(conversion);

  SaveConversions(conversions);

  TriggerAdEvent(conversion.creative_set_id, ConfirmationType::kViewed);

  task_environment_.FastForwardBy(base::TimeDelta::FromDays(3) -
      base::TimeDelta::FromMinutes(1));

  // Act
  ads_->get_conversions()->MaybeConvert("https://foo.bar.com/qux");

  // Assert
  const std::string condition = base::StringPrintf(
      "creative_set_id = '%s' AND confirmation_type = 'conversion'",
          conversion.creative_set_id.c_str());

  ad_events_database_table_->GetIf(condition, [&conversion](
      const Result result,
      const AdEventList& ad_events) {
    ASSERT_EQ(Result::SUCCESS, result);

    EXPECT_EQ(1UL, ad_events.size());
    AdEventInfo ad_event = ad_events.front();

    EXPECT_EQ(conversion.creative_set_id, ad_event.creative_set_id);
  });
}

TEST_F(BatAdsConversionsTest,
    DoNotConvertAdWhenTheConversionHasExpired) {
  // Arrange
  ConversionList conversions;

  ConversionInfo conversion;
  conversion.creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  conversion.type = "postview";
  conversion.url_pattern = "https://www.foo.com/b*r/*";
  conversion.observation_window = 3;
  conversion.expiry_timestamp =
      CalculateExpiryTimestamp(conversion.observation_window);
  conversions.push_back(conversion);

  SaveConversions(conversions);

  TriggerAdEvent(conversion.creative_set_id, ConfirmationType::kViewed);

  task_environment_.FastForwardBy(base::TimeDelta::FromDays(3));

  // Act
  ads_->get_conversions()->MaybeConvert("https://www.foo.com/bar/qux");

  // Assert
  const std::string condition = base::StringPrintf(
      "creative_set_id = '%s' AND confirmation_type = 'conversion'",
          conversion.creative_set_id.c_str());

  ad_events_database_table_->GetIf(condition, [](
      const Result result,
      const AdEventList& ad_events) {
    ASSERT_EQ(Result::SUCCESS, result);

    EXPECT_TRUE(ad_events.empty());
  });
}

}  // namespace ads
