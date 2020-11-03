/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/test/task_environment.h"
#include "bat/ads/internal/ad_serving/ad_notifications/ad_notification_serving.h"
#include "bat/ads/internal/ads_client_mock.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/unittest_util.h"
#include "brave/components/l10n/browser/locale_helper_mock.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "bat/ads/pref_names.h"

using ::testing::_;
using ::testing::AllOf;
using ::testing::Between;
using ::testing::Field;
using ::testing::InvokeArgument;
using ::testing::Matcher;
using ::testing::NiceMock;
using ::testing::Return;

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {
namespace ad_notifications {

namespace {

Matcher<const AdNotificationInfo&> IsNotification(
    const std::string& creative_instance_id) {
  return AllOf(Field("creative_instance_id",
                     &AdNotificationInfo::creative_instance_id,
                     creative_instance_id));
}

}  // namespace

class BatAdsAdNotificationPacingTest : public ::testing::Test {
 protected:
  BatAdsAdNotificationPacingTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME),
        ads_client_mock_(std::make_unique<NiceMock<AdsClientMock>>()),
        ads_(std::make_unique<AdsImpl>(ads_client_mock_.get())),
        locale_helper_mock_(
            std::make_unique<NiceMock<brave_l10n::LocaleHelperMock>>()),
        platform_helper_mock_(
            std::make_unique<NiceMock<PlatformHelperMock>>()) {
    // You can do set-up work for each test here

    brave_l10n::LocaleHelper::GetInstance()->set_for_testing(
        locale_helper_mock_.get());

    PlatformHelper::GetInstance()->set_for_testing(platform_helper_mock_.get());
  }

  ~BatAdsAdNotificationPacingTest() override {
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

    ON_CALL(*locale_helper_mock_, GetLocale()).WillByDefault(Return("en-US"));

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
    SetupTestAds();
  }

  void SetupTestAds() {
    CreativeAdNotificationInfo ad_creative;

    ad_creative.creative_instance_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
    ad_creative.creative_set_id = "c2ba3e7d-f688-4bc4-a053-cbe7ac1e6123";
    ad_creative.campaign_id = "84197fc8-830a-4a8e-8339-7a70c2bfa104";
    ad_creative.start_at_timestamp = DistantPast();
    ad_creative.end_at_timestamp = DistantFuture();
    ad_creative.daily_cap = 1;
    ad_creative.advertiser_id = "5484a63f-eb99-4ba5-a3b0-8c25d3c0e4b2";
    ad_creative.priority = 1;
    ad_creative.per_day = 3;
    ad_creative.total_max = 4;
    ad_creative.category = "Technology & Computing-Software";
    ad_creative.geo_targets = {"US"};
    ad_creative.target_url = "https://brave.com";
    ad_creative.title = "Test Ad 1 Title";
    ad_creative.body = "Test Ad 1 Body";
    ad_creative.ptr = 1.0;

    test_creative_notifications_.push_back(ad_creative);

    ad_creative.creative_instance_id = "a1ac44c2-675f-43e6-ab6d-500614cafe63";
    ad_creative.creative_set_id = "5800049f-cee5-4bcb-90c7-85246d5f5e7c";
    ad_creative.campaign_id = "3d62eca2-324a-4161-a0c5-7d9f29d10ab0";
    ad_creative.start_at_timestamp = DistantPast();
    ad_creative.end_at_timestamp = DistantFuture();
    ad_creative.daily_cap = 1;
    ad_creative.advertiser_id = "9a11b60f-e29d-4446-8d1f-318311e36e0a";
    ad_creative.priority = 2;
    ad_creative.per_day = 3;
    ad_creative.total_max = 4;
    ad_creative.category = "Food & Drink";
    ad_creative.geo_targets = {"US"};
    ad_creative.target_url = "https://brave.com";
    ad_creative.title = "Test Ad 2 Title";
    ad_creative.body = "Test Ad 2 Body";
    ad_creative.ptr = 1.0;

    test_creative_notifications_.push_back(ad_creative);
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
  std::unique_ptr<PlatformHelperMock> platform_helper_mock_;
  std::unique_ptr<Database> database_;

  std::vector<CreativeAdNotificationInfo> test_creative_notifications_;
};

TEST_F(BatAdsAdNotificationPacingTest,
    PacingDisableDelivery) {
  CreativeAdNotificationList creative_ad_notifications;
  test_creative_notifications_[0].ptr = 0;
  creative_ad_notifications.push_back(test_creative_notifications_[0]);

  int iterations = 1000;
  EXPECT_CALL(*ads_client_mock_, ShowNotification(_)).Times(0);

  AdServing ad_serving(ads_.get());
  for (int i = 0; i < iterations; i++) {
    ad_serving.MaybeServeAd(creative_ad_notifications, [](
        const Result result,
        const AdNotificationInfo& ad) {
    });
  }
}

TEST_F(BatAdsAdNotificationPacingTest,
    NoPacing) {
  CreativeAdNotificationList creative_ad_notifications;
  test_creative_notifications_[0].ptr = 1.0;
  creative_ad_notifications.push_back(test_creative_notifications_[0]);

  int iterations = 1000;
  EXPECT_CALL(*ads_client_mock_, ShowNotification(_)).Times(iterations);

  AdServing ad_serving(ads_.get());
  for (int i = 0; i < iterations; i++) {
    ad_serving.MaybeServeAd(creative_ad_notifications, [](
        const Result result,
        const AdNotificationInfo& ad) {
    });
  }
}

TEST_F(BatAdsAdNotificationPacingTest,
    SimplePacing) {
  CreativeAdNotificationList creative_ad_notifications;
  test_creative_notifications_[0].ptr = 0.2;
  creative_ad_notifications.push_back(test_creative_notifications_[0]);

  int iterations = 1000;
  EXPECT_CALL(*ads_client_mock_, ShowNotification(_))
      .Times(Between(iterations * test_creative_notifications_[0].ptr * 0.8,
                     iterations * test_creative_notifications_[0].ptr * 1.2));

  AdServing ad_serving(ads_.get());
  for (int i = 0; i < iterations; i++) {
    ad_serving.MaybeServeAd(creative_ad_notifications, [](
        const Result result,
        const AdNotificationInfo& ad) {
    });
  }
}

TEST_F(BatAdsAdNotificationPacingTest,
    NoPacingPrioritized) {
  CreativeAdNotificationList creative_ad_notifications;
  creative_ad_notifications.push_back(test_creative_notifications_[0]);
  creative_ad_notifications.push_back(test_creative_notifications_[1]);

  EXPECT_CALL(*ads_client_mock_,
              ShowNotification(IsNotification(
                  test_creative_notifications_[0].creative_instance_id)))
      .Times(1);

  AdServing ad_serving(ads_.get());
  ad_serving.MaybeServeAd(creative_ad_notifications, [](
      const Result result,
      const AdNotificationInfo& ad) {
  });
}

TEST_F(BatAdsAdNotificationPacingTest,
    PacingDisableDeliveryPrioritized) {
  CreativeAdNotificationList creative_ad_notifications;
  test_creative_notifications_[0].ptr = 0;
  creative_ad_notifications.push_back(test_creative_notifications_[0]);
  creative_ad_notifications.push_back(test_creative_notifications_[1]);

  EXPECT_CALL(*ads_client_mock_,
              ShowNotification(IsNotification(
                  test_creative_notifications_[1].creative_instance_id)))
      .Times(1);

  AdServing ad_serving(ads_.get());
  ad_serving.MaybeServeAd(creative_ad_notifications, [](
      const Result result,
      const AdNotificationInfo& ad) {
  });
}

TEST_F(BatAdsAdNotificationPacingTest,
    PacingAndPrioritization) {
  CreativeAdNotificationList creative_ad_notifications;
  test_creative_notifications_[0].ptr = 0.5;
  creative_ad_notifications.push_back(test_creative_notifications_[0]);
  test_creative_notifications_[1].ptr = 0.5;
  creative_ad_notifications.push_back(test_creative_notifications_[1]);

  int iterations = 1000;

  EXPECT_CALL(*ads_client_mock_,
              ShowNotification(IsNotification(
                  test_creative_notifications_[0].creative_instance_id)))
      .Times(Between(iterations * test_creative_notifications_[0].ptr * 0.8,
                     iterations * test_creative_notifications_[0].ptr * 1.2));

  // test_creative_notifications_[1] ad would be shown probabilistically when
  // test_creative_notifications_[0] gets dropped due to pacing.
  EXPECT_CALL(*ads_client_mock_,
              ShowNotification(IsNotification(
                  test_creative_notifications_[1].creative_instance_id)))
      .Times(Between(iterations * test_creative_notifications_[0].ptr * 0.8 *
                         test_creative_notifications_[1].ptr,
                     iterations * test_creative_notifications_[0].ptr * 1.2 *
                         test_creative_notifications_[1].ptr));

  AdServing ad_serving(ads_.get());
  for (int i = 0; i < iterations; i++) {
    ad_serving.MaybeServeAd(creative_ad_notifications, [](
        const Result result,
        const AdNotificationInfo& ad) {
    });
  }
}

}  // namespace ad_notifications
}  // namespace ads
