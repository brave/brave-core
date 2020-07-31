/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/test/task_environment.h"
#include "bat/ads/internal/ads_client_mock.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/unittest_utils.h"
#include "brave/components/l10n/browser/locale_helper_mock.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::_;
using ::testing::AllOf;
using ::testing::Between;
using ::testing::Field;
using ::testing::InvokeArgument;
using ::testing::Matcher;
using ::testing::NiceMock;
using ::testing::Pointee;
using ::testing::Return;

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {

Matcher<AdNotificationInfo> IsNotification(
    const std::string& creative_instance_id) {
  return AllOf(Field("creative_instance_id",
                     &AdNotificationInfo::creative_instance_id,
                     creative_instance_id));
}

}  // namespace

class BatAdsPacingTest : public ::testing::Test {
 protected:
  BatAdsPacingTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME),
        ads_client_mock_(std::make_unique<NiceMock<AdsClientMock>>()),
        ads_(std::make_unique<AdsImpl>(ads_client_mock_.get())),
        locale_helper_mock_(
            std::make_unique<NiceMock<brave_l10n::LocaleHelperMock>>()) {
    // You can do set-up work for each test here

    brave_l10n::LocaleHelper::GetInstance()->set_for_testing(
        locale_helper_mock_.get());
  }

  ~BatAdsPacingTest() override {
    // You can do clean-up work that doesn't throw exceptions here
  }

  // If the constructor and destructor are not enough for setting up and
  // cleaning up each test, you can use the following methods

  void SetUp() override {
    // Code here will be called immediately after the constructor (right before
    // each test)

    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    const base::FilePath path = temp_dir_.GetPath();

    ON_CALL(*ads_client_mock_, IsEnabled()).WillByDefault(Return(true));

    ON_CALL(*locale_helper_mock_, GetLocale()).WillByDefault(Return("en-US"));

    MockLoad(ads_client_mock_);
    MockLoadUserModelForId(ads_client_mock_);
    MockLoadResourceForId(ads_client_mock_);
    MockSave(ads_client_mock_);

    database_ = std::make_unique<Database>(path.AppendASCII("database.sqlite"));
    MockRunDBTransaction(ads_client_mock_, database_);

    Initialize(ads_);

    info_1_.creative_instance_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
    info_1_.creative_set_id = "c2ba3e7d-f688-4bc4-a053-cbe7ac1e6123";
    info_1_.campaign_id = "84197fc8-830a-4a8e-8339-7a70c2bfa104";
    info_1_.start_at_timestamp = DistantPast();
    info_1_.end_at_timestamp = DistantFuture();
    info_1_.daily_cap = 1;
    info_1_.advertiser_id = "5484a63f-eb99-4ba5-a3b0-8c25d3c0e4b2";
    info_1_.priority = 1;
    info_1_.per_day = 3;
    info_1_.total_max = 4;
    info_1_.category = "Technology & Computing-Software";
    info_1_.geo_targets = {"US"};
    info_1_.target_url = "https://brave.com";
    info_1_.title = "Test Ad 1 Title";
    info_1_.body = "Test Ad 1 Body";
    info_1_.ptr = 1.0;

    info_2_.creative_instance_id = "a1ac44c2-675f-43e6-ab6d-500614cafe63";
    info_2_.creative_set_id = "5800049f-cee5-4bcb-90c7-85246d5f5e7c";
    info_2_.campaign_id = "3d62eca2-324a-4161-a0c5-7d9f29d10ab0";
    info_2_.start_at_timestamp = DistantPast();
    info_2_.end_at_timestamp = DistantFuture();
    info_2_.daily_cap = 1;
    info_2_.advertiser_id = "9a11b60f-e29d-4446-8d1f-318311e36e0a";
    info_2_.priority = 2;
    info_2_.per_day = 3;
    info_2_.total_max = 4;
    info_2_.category = "Food & Drink";
    info_2_.geo_targets = {"US"};
    info_2_.target_url = "https://brave.com";
    info_2_.title = "Test Ad 2 Title";
    info_2_.body = "Test Ad 2 Body";
    info_2_.ptr = 1.0;
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
  std::unique_ptr<Database> database_;

  CreativeAdNotificationInfo info_1_, info_2_;
};

TEST_F(BatAdsPacingTest, PacingDisableDelivery) {
  CreativeAdNotificationList creative_ad_notifications;
  info_1_.ptr = 0;
  creative_ad_notifications.push_back(info_1_);

  int iterations = 1000;
  EXPECT_CALL(*ads_client_mock_, ShowNotification(_)).Times(0);

  for (int i = 0; i < iterations; i++) {
    ads_->ServeAdNotificationWithPacing(creative_ad_notifications);
  }
}

TEST_F(BatAdsPacingTest, NoPacing) {
  CreativeAdNotificationList creative_ad_notifications;
  info_1_.ptr = 1.0;
  creative_ad_notifications.push_back(info_1_);

  int iterations = 1000;
  EXPECT_CALL(*ads_client_mock_, ShowNotification(_)).Times(iterations);

  for (int i = 0; i < iterations; i++) {
    ads_->ServeAdNotificationWithPacing(creative_ad_notifications);
  }
}

TEST_F(BatAdsPacingTest, SimplePacing) {
  CreativeAdNotificationList creative_ad_notifications;
  info_1_.ptr = 0.2;
  creative_ad_notifications.push_back(info_1_);

  int iterations = 1000;
  EXPECT_CALL(*ads_client_mock_, ShowNotification(_))
      .Times(Between(iterations * info_1_.ptr * 0.8,
                     iterations * info_1_.ptr * 1.2));

  for (int i = 0; i < iterations; i++) {
    ads_->ServeAdNotificationWithPacing(creative_ad_notifications);
  }
}

TEST_F(BatAdsPacingTest, NoPacingPrioritized) {
  CreativeAdNotificationList creative_ad_notifications;
  creative_ad_notifications.push_back(info_1_);
  creative_ad_notifications.push_back(info_2_);

  EXPECT_CALL(
      *ads_client_mock_,
      ShowNotification(Pointee(IsNotification(info_1_.creative_instance_id))))
      .Times(1);

  ads_->ServeAdNotificationWithPacing(creative_ad_notifications);
}

TEST_F(BatAdsPacingTest, PacingDisableDeliveryPrioritized) {
  CreativeAdNotificationList creative_ad_notifications;
  info_1_.ptr = 0;
  creative_ad_notifications.push_back(info_1_);
  creative_ad_notifications.push_back(info_2_);

  EXPECT_CALL(
      *ads_client_mock_,
      ShowNotification(Pointee(IsNotification(info_2_.creative_instance_id))))
      .Times(1);

  ads_->ServeAdNotificationWithPacing(creative_ad_notifications);
}

TEST_F(BatAdsPacingTest, PacingAndPrioritization) {
  CreativeAdNotificationList creative_ad_notifications;
  info_1_.ptr = 0.5;
  creative_ad_notifications.push_back(info_1_);
  info_2_.ptr = 0.5;
  creative_ad_notifications.push_back(info_2_);

  int iterations = 1000;

  EXPECT_CALL(
      *ads_client_mock_,
      ShowNotification(Pointee(IsNotification(info_1_.creative_instance_id))))
      .Times(Between(iterations * info_1_.ptr * 0.8,
                     iterations * info_1_.ptr * 1.2));

  // info_2_ ad would be shown probabilistically when info_1_ gets dropped due
  // to pacing.
  EXPECT_CALL(
      *ads_client_mock_,
      ShowNotification(Pointee(IsNotification(info_2_.creative_instance_id))))
      .Times(Between(iterations * info_1_.ptr * 0.8 * info_2_.ptr,
                     iterations * info_1_.ptr * 1.2 * info_2_.ptr));

  for (int i = 0; i < iterations; i++) {
    ads_->ServeAdNotificationWithPacing(creative_ad_notifications);
  }
}

}  // namespace ads
