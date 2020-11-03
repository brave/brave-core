/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/eligible_ads/ad_notifications/eligible_ad_notifications.h"

#include <memory>
#include <vector>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/task_environment.h"
#include "brave/components/l10n/browser/locale_helper_mock.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "bat/ads/internal/ads_client_mock.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/client/client.h"
#include "bat/ads/internal/container_util.h"
#include "bat/ads/internal/platform/platform_helper_mock.h"
#include "bat/ads/internal/time_util.h"
#include "bat/ads/internal/unittest_util.h"
#include "bat/ads/pref_names.h"

// npm run test -- brave_unit_tests --filter=BatAds*

using ::testing::NiceMock;
using ::testing::Return;

namespace ads {
namespace ad_notifications {

class BatAdsEligibleAdNotificationsTest : public ::testing::Test {
 protected:
  BatAdsEligibleAdNotificationsTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME),
        ads_client_mock_(std::make_unique<NiceMock<AdsClientMock>>()),
        ads_(std::make_unique<AdsImpl>(ads_client_mock_.get())),
        eligible_ads_(std::make_unique<EligibleAds>(ads_.get())),
        locale_helper_mock_(std::make_unique<
            NiceMock<brave_l10n::LocaleHelperMock>>()),
        platform_helper_mock_(std::make_unique<
            NiceMock<PlatformHelperMock>>()) {
    // You can do set-up work for each test here

    brave_l10n::LocaleHelper::GetInstance()->set_for_testing(
        locale_helper_mock_.get());

    PlatformHelper::GetInstance()->set_for_testing(platform_helper_mock_.get());
  }

  ~BatAdsEligibleAdNotificationsTest() override {
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

  CreativeAdNotificationList GetAds(
      const int count) {
    CreativeAdNotificationList ads;

    for (int i = 0; i < count; i++) {
      CreativeAdNotificationInfo ad;

      const int creative_instance_id = i + 1;
      ad.creative_instance_id = base::NumberToString(creative_instance_id);

      ad.daily_cap = 1;

      const int advertiser_id = 1 + (i / 2);
      ad.advertiser_id = base::NumberToString(advertiser_id);

      ad.per_day = 1;
      ad.total_max = 1;

      ads.push_back(ad);
    }

    return ads;
  }

  base::test::TaskEnvironment task_environment_;

  base::ScopedTempDir temp_dir_;

  std::unique_ptr<AdsClientMock> ads_client_mock_;
  std::unique_ptr<AdsImpl> ads_;
  std::unique_ptr<EligibleAds> eligible_ads_;
  std::unique_ptr<brave_l10n::LocaleHelperMock> locale_helper_mock_;
  std::unique_ptr<PlatformHelperMock> platform_helper_mock_;
  std::unique_ptr<Database> database_;
};

TEST_F(BatAdsEligibleAdNotificationsTest,
    NoSeenAdvertisersOrAds) {
  // Arrange
  const CreativeAdNotificationList ads = GetAds(6);

  const CreativeAdInfo last_delivered_ad;

  // Act
  const CreativeAdNotificationList eligible_ads =
      eligible_ads_->Get(ads, last_delivered_ad, {});

  // Assert
  const CreativeAdNotificationList expected_ads = ads;
  EXPECT_TRUE(CompareAsSets(expected_ads, eligible_ads));
}

TEST_F(BatAdsEligibleAdNotificationsTest,
    EligibleAd) {
  // Arrange
  const CreativeAdNotificationList ads = GetAds(6);

  const CreativeAdInfo last_delivered_ad;

  ads_->get_client()->UpdateSeenAdNotification("1");
  ads_->get_client()->UpdateSeenAdNotification("2");
  ads_->get_client()->UpdateSeenAdNotification("3");
  ads_->get_client()->UpdateSeenAdNotification("4");
  ads_->get_client()->UpdateSeenAdNotification("5");

  // Act
  const CreativeAdNotificationList eligible_ads =
      eligible_ads_->Get(ads, last_delivered_ad, {});

  // Assert
  CreativeAdNotificationInfo ad;
  ad.creative_instance_id = "6";
  ad.advertiser_id = "3";

  const CreativeAdNotificationList expected_ads = {
    ad
  };

  EXPECT_TRUE(CompareAsSets(expected_ads, eligible_ads));
}

TEST_F(BatAdsEligibleAdNotificationsTest,
    EligibleAds) {
  // Arrange
  const CreativeAdNotificationList ads = GetAds(6);

  const CreativeAdInfo last_delivered_ad;

  ads_->get_client()->UpdateSeenAdNotification("1");
  ads_->get_client()->UpdateSeenAdNotification("2");
  ads_->get_client()->UpdateSeenAdNotification("4");
  ads_->get_client()->UpdateSeenAdNotification("5");

  // Act
  const CreativeAdNotificationList eligible_ads =
      eligible_ads_->Get(ads, last_delivered_ad, {});

  // Assert
  CreativeAdNotificationInfo ad_1;
  ad_1.creative_instance_id = "3";
  ad_1.advertiser_id = "2";

  CreativeAdNotificationInfo ad_2;
  ad_2.creative_instance_id = "6";
  ad_2.advertiser_id = "3";

  const CreativeAdNotificationList expected_ads = {
    ad_1,
    ad_2
  };

  EXPECT_TRUE(CompareAsSets(expected_ads, eligible_ads));
}

TEST_F(BatAdsEligibleAdNotificationsTest,
    EligibleAdsRoundRobin) {
  // Arrange
  const CreativeAdNotificationList ads = GetAds(6);

  const CreativeAdInfo last_delivered_ad;

  ads_->get_client()->UpdateSeenAdNotification("1");
  ads_->get_client()->UpdateSeenAdNotification("2");
  ads_->get_client()->UpdateSeenAdNotification("3");
  ads_->get_client()->UpdateSeenAdNotification("4");
  ads_->get_client()->UpdateSeenAdNotification("5");
  ads_->get_client()->UpdateSeenAdNotification("6");

  // Act
  const CreativeAdNotificationList eligible_ads =
      eligible_ads_->Get(ads, last_delivered_ad, {});

  // Assert
  const CreativeAdNotificationList expected_ads = ads;
  EXPECT_TRUE(CompareAsSets(expected_ads, eligible_ads));
}

TEST_F(BatAdsEligibleAdNotificationsTest,
    EligibleAdvertiser) {
  // Arrange
  const CreativeAdNotificationList ads = GetAds(6);

  const CreativeAdInfo last_delivered_ad;

  ads_->get_client()->UpdateSeenAdvertiser("1");
  ads_->get_client()->UpdateSeenAdvertiser("2");

  // Act
  const CreativeAdNotificationList eligible_ads =
      eligible_ads_->Get(ads, last_delivered_ad, {});

  // Assert
  CreativeAdNotificationInfo ad_1;
  ad_1.creative_instance_id = "5";
  ad_1.advertiser_id = "3";

  CreativeAdNotificationInfo ad_2;
  ad_2.creative_instance_id = "6";
  ad_2.advertiser_id = "3";

  const CreativeAdNotificationList expected_ads = {
    ad_1,
    ad_2
  };

  EXPECT_TRUE(CompareAsSets(expected_ads, eligible_ads));
}

TEST_F(BatAdsEligibleAdNotificationsTest,
    EligibleAdvertisers) {
  // Arrange
  const CreativeAdNotificationList ads = GetAds(6);

  const CreativeAdInfo last_delivered_ad;

  ads_->get_client()->UpdateSeenAdvertiser("1");

  // Act
  const CreativeAdNotificationList eligible_ads =
      eligible_ads_->Get(ads, last_delivered_ad, {});

  // Assert
  CreativeAdNotificationInfo ad_1;
  ad_1.creative_instance_id = "3";
  ad_1.advertiser_id = "2";

  CreativeAdNotificationInfo ad_2;
  ad_2.creative_instance_id = "4";
  ad_2.advertiser_id = "2";

  CreativeAdNotificationInfo ad_3;
  ad_3.creative_instance_id = "5";
  ad_3.advertiser_id = "3";

  CreativeAdNotificationInfo ad_4;
  ad_4.creative_instance_id = "6";
  ad_4.advertiser_id = "3";

  const CreativeAdNotificationList expected_ads = {
    ad_1,
    ad_2,
    ad_3,
    ad_4
  };

  EXPECT_TRUE(CompareAsSets(expected_ads, eligible_ads));
}

TEST_F(BatAdsEligibleAdNotificationsTest,
    EligibleAdvertisersRoundRobin) {
  // Arrange
  const CreativeAdNotificationList ads = GetAds(6);

  const CreativeAdInfo last_delivered_ad;

  ads_->get_client()->UpdateSeenAdvertiser("1");
  ads_->get_client()->UpdateSeenAdvertiser("2");
  ads_->get_client()->UpdateSeenAdvertiser("3");

  // Act
  const CreativeAdNotificationList eligible_ads =
      eligible_ads_->Get(ads, last_delivered_ad, {});

  // Assert
  const CreativeAdNotificationList expected_ads = ads;
  EXPECT_TRUE(CompareAsSets(expected_ads, eligible_ads));
}

TEST_F(BatAdsEligibleAdNotificationsTest,
    RoundRobin) {
  // Arrange
  const CreativeAdNotificationList ads = GetAds(6);

  const CreativeAdInfo last_delivered_ad;

  ads_->get_client()->UpdateSeenAdNotification("1");
  ads_->get_client()->UpdateSeenAdNotification("2");
  ads_->get_client()->UpdateSeenAdvertiser("1");
  ads_->get_client()->UpdateSeenAdNotification("3");
  ads_->get_client()->UpdateSeenAdNotification("4");
  ads_->get_client()->UpdateSeenAdvertiser("2");
  ads_->get_client()->UpdateSeenAdNotification("5");
  ads_->get_client()->UpdateSeenAdNotification("6");
  ads_->get_client()->UpdateSeenAdvertiser("3");

  // Act
  const CreativeAdNotificationList eligible_ads =
      eligible_ads_->Get(ads, last_delivered_ad, {});

  // Assert
  const CreativeAdNotificationList expected_ads = ads;
  EXPECT_TRUE(CompareAsSets(expected_ads, eligible_ads));
}

TEST_F(BatAdsEligibleAdNotificationsTest,
    LastDeliveredAd) {
  // Arrange
  const CreativeAdNotificationList ads = GetAds(2);

  CreativeAdInfo last_delivered_ad;
  last_delivered_ad.advertiser_id = "1";
  last_delivered_ad.creative_instance_id = "1";

  // Act
  const CreativeAdNotificationList eligible_ads =
      eligible_ads_->Get(ads, last_delivered_ad, {});

  // Assert
  CreativeAdNotificationInfo ad;
  ad.creative_instance_id = "2";
  ad.advertiser_id = "1";

  const CreativeAdNotificationList expected_ads = {
    ad
  };

  EXPECT_TRUE(CompareAsSets(expected_ads, eligible_ads));
}

TEST_F(BatAdsEligibleAdNotificationsTest,
    LastDeliveredAdForSingleAd) {
  // Arrange
  const CreativeAdNotificationList ads = GetAds(2);

  CreativeAdInfo last_delivered_ad;
  last_delivered_ad.advertiser_id = "1";
  last_delivered_ad.creative_instance_id = "1";

  // Act
  const CreativeAdNotificationList eligible_ads =
      eligible_ads_->Get(ads, last_delivered_ad, {});

  // Assert
  CreativeAdNotificationInfo ad;
  ad.creative_instance_id = "1";
  ad.advertiser_id = "1";

  const CreativeAdNotificationList expected_ads = {
    ad
  };

  EXPECT_TRUE(CompareAsSets(expected_ads, eligible_ads));
}

}  // namespace ad_notifications
}  // namespace ads
