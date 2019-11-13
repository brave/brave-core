/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>

#include "testing/gtest/include/gtest/gtest.h"

#include "bat/ads/internal/filters/ad_history_confirmation_filter.h"

#include "bat/ads/ads_history.h"
#include "bat/ads/ad_history_detail.h"
#include "bat/ads/internal/client_mock.h"
#include "bat/ads/internal/ads_client_mock.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/time.h"

// npm run test -- brave_unit_tests --filter=Ads*

using std::placeholders::_1;
using ::testing::_;
using ::testing::Invoke;

namespace {

const std::vector<std::string> kTestAdUuids = {
  "ab9deba5-01bf-492b-9bb8-7bc4318fe272",
  "a577e7fe-d86c-4997-bbaa-4041dfd4075c",
  "a6326b14-e4f4-4597-a358-ae6134eb26c1",
};

}  // namespace

namespace ads {

class BraveAdsAdHistoryConfirmationFilterTest : public ::testing::Test {
 protected:
  BraveAdsAdHistoryConfirmationFilterTest()
  : mock_ads_client_(std::make_unique<MockAdsClient>()),
    ads_(std::make_unique<AdsImpl>(mock_ads_client_.get())) {
    // You can do set-up work for each test here
  }
  ~BraveAdsAdHistoryConfirmationFilterTest() override {
    // You can do clean-up work that doesn't throw exceptions here
  }

  // If the constructor and destructor are not enough for setting up and
  // cleaning up each test, you can use the following methods

  void SetUp() override {
    // Code here will be called immediately after the constructor (right before
    // each test)

    auto callback = std::bind(
        &BraveAdsAdHistoryConfirmationFilterTest::OnAdsImplInitialize, this,
        _1);
    ads_->Initialize(callback);

    client_mock_ = std::make_unique<ClientMock>(ads_.get(),
        mock_ads_client_.get());

    ad_history_filter_ = std::make_unique<AdHistoryConfirmationFilter>();

    ads_history_.clear();
  }

  void OnAdsImplInitialize(const Result result) {
    EXPECT_EQ(Result::SUCCESS, result);
  }

  void TearDown() override {
    // Code here will be called immediately after each test (right before the
    // destructor)
  }

  void PopulateAdHistory(const std::string& ad_uuid,
      const ConfirmationType::Value* values, const uint32_t items,
      const uint64_t time_offset_per_item) {
    AdHistoryDetail ad_history_detail;

    auto now_in_seconds = Time::NowInSeconds();

    for (unsigned int i = 0; i < items; i++) {
      ad_history_detail.ad_content.uuid = ad_uuid;
      ad_history_detail.timestamp_in_seconds = now_in_seconds;
      ad_history_detail.ad_content.ad_action = ConfirmationType(values[i]);

      ads_history_.push_back(ad_history_detail);

      now_in_seconds += time_offset_per_item;
    }
  }

  bool IsConfirmationTypeOfInterest(const ConfirmationType& confirmation_type) {
    bool is_of_interest = false;

    if ((confirmation_type == ConfirmationType::Value::CLICK)
        || (confirmation_type == ConfirmationType::Value::VIEW)
        || (confirmation_type == ConfirmationType::Value::DISMISS)) {
      is_of_interest = true;
    }

    return is_of_interest;
  }

  void TestFiltering(const std::string& ad_uuid,
      ConfirmationType::Value expected_confirmation_type_value) {
    std::map<std::string, AdHistoryDetail> ad_history_detail_map;

    for (const AdHistoryDetail& adHistoryDetail : ads_history_filtered_) {
      EXPECT_TRUE(IsConfirmationTypeOfInterest(
          adHistoryDetail.ad_content.ad_action));

      if (adHistoryDetail.ad_content.uuid == ad_uuid) {
        ad_history_detail_map[adHistoryDetail.ad_content.uuid] =
            adHistoryDetail;
      }
    }

    const AdHistoryDetail& ad_history_detail = ad_history_detail_map[ad_uuid];
    EXPECT_EQ(ad_history_detail.ad_content.ad_action.value(),
        expected_confirmation_type_value);
  }

  void TestFilteringWithTimestamps(const std::string& ad_uuid,
      uint64_t expected_timestamp_in_seconds,
      ConfirmationType::Value expected_confirmation_type_value) {
    std::map<std::string, AdHistoryDetail> ad_history_detail_map;

    for (const AdHistoryDetail& adHistoryDetail : ads_history_filtered_) {
      EXPECT_TRUE(IsConfirmationTypeOfInterest(
          adHistoryDetail.ad_content.ad_action));

      if (adHistoryDetail.ad_content.uuid == ad_uuid) {
        ad_history_detail_map[adHistoryDetail.ad_content.uuid] =
            adHistoryDetail;
      }
    }

    const AdHistoryDetail& ad_history_detail = ad_history_detail_map[ad_uuid];
    EXPECT_EQ(ad_history_detail.timestamp_in_seconds,
        expected_timestamp_in_seconds);
    EXPECT_EQ(ad_history_detail.ad_content.ad_action.value(),
        expected_confirmation_type_value);
  }

  void PerformBasicUnitTest(const std::string& ad_uuid,
      const ConfirmationType::Value* values, const uint32_t items,
      const ConfirmationType::Value expected_confirmation_value) {
    PopulateAdHistory(ad_uuid, values, items, 1);

    const AdHistoryDetail& expected_ad_history_detail =
        ads_history_[1];  // Trump
    const uint64_t expected_timestamp_in_seconds =
        expected_ad_history_detail.timestamp_in_seconds;

    // Act
    ads_history_filtered_ = ad_history_filter_->ApplyFilter(ads_history_);

    // Assert
    TestFilteringWithTimestamps(ad_uuid, expected_timestamp_in_seconds,
        expected_confirmation_value);
  }

  std::unique_ptr<MockAdsClient> mock_ads_client_;
  std::unique_ptr<AdsImpl> ads_;

  std::unique_ptr<ClientMock> client_mock_;

  std::deque<AdHistoryDetail> ads_history_;
  std::deque<AdHistoryDetail> ads_history_filtered_;
  std::unique_ptr<AdHistoryFilter> ad_history_filter_;
};

TEST_F(BraveAdsAdHistoryConfirmationFilterTest,
    NoFilteredResultsWhenNoAds) {
  // Arrange
  ConfirmationType::Value confirmation_types[] = {
  };

  size_t size_of_confirmation_types = sizeof(confirmation_types) /
      sizeof(ConfirmationType::Value);
  PopulateAdHistory(kTestAdUuids[0], confirmation_types,
      size_of_confirmation_types, 1);

  // Act
  ads_history_filtered_ = ad_history_filter_->ApplyFilter(ads_history_);

  // Assert
  EXPECT_EQ(ads_history_filtered_.size(), (uint64_t)0);
}

TEST_F(BraveAdsAdHistoryConfirmationFilterTest,
    NoFilteredResultsForUnrecognisedConfirmationTypes) {
  // Arrange
  ConfirmationType::Value confirmation_types[] = {
    ConfirmationType::Value::UNKNOWN,
    ConfirmationType::Value::FLAG,
    ConfirmationType::Value::UPVOTE,
    ConfirmationType::Value::DOWNVOTE,
    ConfirmationType::Value::LANDED,
  };

  size_t size_of_confirmation_types = sizeof(confirmation_types) /
      sizeof(ConfirmationType::Value);
  PopulateAdHistory(kTestAdUuids[0], confirmation_types,
      size_of_confirmation_types, 1);

  // Act
  ads_history_filtered_ = ad_history_filter_->ApplyFilter(ads_history_);

  // Assert
  EXPECT_EQ(ads_history_.size(), (uint64_t)5);
  EXPECT_EQ(ads_history_filtered_.size(), (uint64_t)0);
}

TEST_F(BraveAdsAdHistoryConfirmationFilterTest,
    FilteredDismissResultWithUnrecognisedConfirmationTypes) {
  // Arrange
  ConfirmationType::Value confirmation_types[] = {
    ConfirmationType::Value::UNKNOWN,
    ConfirmationType::Value::FLAG,
    ConfirmationType::Value::DISMISS,  // Trump
    ConfirmationType::Value::UPVOTE,
    ConfirmationType::Value::DOWNVOTE,
    ConfirmationType::Value::LANDED,
  };

  size_t size_of_confirmation_types = sizeof(confirmation_types) /
      sizeof(ConfirmationType::Value);
  PopulateAdHistory(kTestAdUuids[0], confirmation_types,
      size_of_confirmation_types, 1);

  const AdHistoryDetail& expected_ad_history_detail =
      ads_history_[2];  // ::DISMISS
  const uint64_t expected_timestamp =
      expected_ad_history_detail.timestamp_in_seconds;

  // Act
  ads_history_filtered_ = ad_history_filter_->ApplyFilter(ads_history_);

  // Assert
  EXPECT_EQ(ads_history_.size(), (uint64_t)6);
  EXPECT_EQ(ads_history_filtered_.size(), (uint64_t)1);
  const AdHistoryDetail& ad_history_detail = ads_history_filtered_.front();
  EXPECT_EQ(ad_history_detail.timestamp_in_seconds, expected_timestamp);
  EXPECT_EQ(ad_history_detail.ad_content.ad_action,
       ConfirmationType::Value::DISMISS);
}

TEST_F(BraveAdsAdHistoryConfirmationFilterTest,
    ExpectLatestDismiss) {
  // Arrange
  ConfirmationType::Value confirmation_types[] = {
    ConfirmationType::Value::DISMISS,
    ConfirmationType::Value::DISMISS,  // Trump
  };

  size_t size_of_confirmation_types = sizeof(confirmation_types) /
      sizeof(ConfirmationType::Value);
  PopulateAdHistory(kTestAdUuids[0], confirmation_types,
      size_of_confirmation_types, 1);

  const AdHistoryDetail& expected_ad_history_detail =
      ads_history_.back();  // Trump
  const uint64_t expected_timestamp =
      expected_ad_history_detail.timestamp_in_seconds;

  // Act
  ads_history_filtered_ = ad_history_filter_->ApplyFilter(ads_history_);

  // Assert
  EXPECT_EQ(ads_history_.size(), (uint64_t)2);
  EXPECT_EQ(ads_history_filtered_.size(), (uint64_t)1);
  const AdHistoryDetail& ad_history_detail = ads_history_filtered_.front();
  EXPECT_EQ(ad_history_detail.timestamp_in_seconds, expected_timestamp);
  EXPECT_EQ(ad_history_detail.ad_content.ad_action,
       ConfirmationType::Value::DISMISS);
}

TEST_F(BraveAdsAdHistoryConfirmationFilterTest,
    ViewTrumpsDismiss) {
  // Arrange
  const ConfirmationType::Value expected_confirmation_type =
    ConfirmationType::Value::VIEW;

  ConfirmationType::Value confirmation_types[] = {
    ConfirmationType::Value::DISMISS,
    expected_confirmation_type,
    ConfirmationType::Value::DISMISS,
  };

  size_t size_of_confirmation_types = sizeof(confirmation_types) /
      sizeof(ConfirmationType::Value);

  PerformBasicUnitTest(kTestAdUuids[0], confirmation_types,
      size_of_confirmation_types, expected_confirmation_type);
}

TEST_F(BraveAdsAdHistoryConfirmationFilterTest,
    ClickTrumpsDismiss) {
  // Arrange
  const ConfirmationType::Value expected_confirmation_type =
    ConfirmationType::Value::CLICK;

  ConfirmationType::Value confirmation_types[] = {
    ConfirmationType::Value::DISMISS,
    expected_confirmation_type,
    ConfirmationType::Value::DISMISS,
  };

  size_t size_of_confirmation_types = sizeof(confirmation_types) /
      sizeof(ConfirmationType::Value);

  PerformBasicUnitTest(kTestAdUuids[0], confirmation_types,
      size_of_confirmation_types, expected_confirmation_type);
}

TEST_F(BraveAdsAdHistoryConfirmationFilterTest,
    ClickTrumpsView) {
  // Arrange
  const ConfirmationType::Value expected_confirmation_type =
    ConfirmationType::Value::CLICK;

  ConfirmationType::Value confirmation_types[] = {
    ConfirmationType::Value::VIEW,
    expected_confirmation_type,
    ConfirmationType::Value::VIEW,
  };

  size_t size_of_confirmation_types = sizeof(confirmation_types) /
      sizeof(ConfirmationType::Value);

  PerformBasicUnitTest(kTestAdUuids[0], confirmation_types,
      size_of_confirmation_types, expected_confirmation_type);
}

TEST_F(BraveAdsAdHistoryConfirmationFilterTest,
    ClickTrumpsViewAndDismiss) {
  // Arrange
  const ConfirmationType::Value expected_confirmation_type =
    ConfirmationType::Value::CLICK;

  ConfirmationType::Value confirmation_types[] = {
    ConfirmationType::Value::DISMISS,
    expected_confirmation_type,
    ConfirmationType::Value::VIEW,
  };

  size_t size_of_confirmation_types = sizeof(confirmation_types) /
      sizeof(ConfirmationType::Value);

  PerformBasicUnitTest(kTestAdUuids[0], confirmation_types,
      size_of_confirmation_types, expected_confirmation_type);
}

TEST_F(BraveAdsAdHistoryConfirmationFilterTest,
    MultipleAdHistoriesFilterCorrectly) {
  // Arrange
  size_t size_of_confirmation_types = 0;

  ConfirmationType::Value confirmationTypesForAd1[] = {
    ConfirmationType::Value::DISMISS,
    ConfirmationType::Value::DISMISS,
    ConfirmationType::Value::VIEW,  // Trump
    ConfirmationType::Value::DISMISS,
  };

  size_of_confirmation_types = sizeof(confirmationTypesForAd1) /
      sizeof(ConfirmationType::Value);
  PopulateAdHistory(kTestAdUuids[0], confirmationTypesForAd1,
      size_of_confirmation_types, 1);

  ConfirmationType::Value confirmationTypesForAd2[] = {
    ConfirmationType::Value::DISMISS,
    ConfirmationType::Value::CLICK,  // Trump
    ConfirmationType::Value::VIEW,
    ConfirmationType::Value::DISMISS,
  };

  size_of_confirmation_types = sizeof(confirmationTypesForAd2) /
      sizeof(ConfirmationType::Value);
  PopulateAdHistory(kTestAdUuids[1], confirmationTypesForAd2,
      size_of_confirmation_types, 1);

  ConfirmationType::Value confirmationTypesForAd3[] = {
    ConfirmationType::Value::CLICK,  // Trump
    ConfirmationType::Value::VIEW,
    ConfirmationType::Value::DISMISS,
  };

  size_of_confirmation_types = sizeof(confirmationTypesForAd3) /
      sizeof(ConfirmationType::Value);
  PopulateAdHistory(kTestAdUuids[2], confirmationTypesForAd3,
      size_of_confirmation_types, 1);

  // Act
  ads_history_filtered_ =
      ad_history_filter_->ApplyFilter(ads_history_);

  // Assert
  TestFiltering(kTestAdUuids[0], ConfirmationType::Value::VIEW);
  TestFiltering(kTestAdUuids[1], ConfirmationType::Value::CLICK);
  TestFiltering(kTestAdUuids[2], ConfirmationType::Value::CLICK);
}

}  // namespace ads
