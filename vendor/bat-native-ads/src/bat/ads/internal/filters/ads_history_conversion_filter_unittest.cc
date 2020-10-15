/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/filters/ads_history_conversion_filter.h"

#include <deque>

#include "testing/gtest/include/gtest/gtest.h"
#include "bat/ads/ad_history.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

TEST(BatAdsHistoryConversionFilterTest,
    FilterUnsupportedActions) {
  // Arrange
  AdHistory ad1;  // Unsupported
  ad1.ad_content.uuid = "69b684d7-d893-4f4e-b156-859919a0fcc9";
  ad1.ad_content.type = AdContent::AdType::kAdNotification;
  ad1.ad_content.creative_instance_id = "69b684d7-d893-4f4e-b156-859919a0fcc9";
  ad1.ad_content.ad_action = ConfirmationType::kLanded;

  AdHistory ad2;  // Unsupported
  ad2.ad_content.uuid = "d3be2e79-ffa8-4b4e-b61e-88545055fbad";
  ad2.ad_content.type = AdContent::AdType::kAdNotification;
  ad2.ad_content.creative_instance_id = "d3be2e79-ffa8-4b4e-b61e-88545055fbad";
  ad2.ad_content.ad_action = ConfirmationType::kFlagged;

  AdHistory ad3;  // Unsupported
  ad3.ad_content.uuid = "9390f66a-d4f2-4c8a-8315-1baed4aae612";
  ad3.ad_content.type = AdContent::AdType::kAdNotification;
  ad3.ad_content.creative_instance_id = "9390f66a-d4f2-4c8a-8315-1baed4aae612";
  ad3.ad_content.ad_action = ConfirmationType::kUpvoted;

  AdHistory ad4;  // Unsupported
  ad4.ad_content.uuid = "47c73793-d1c1-4fdb-8530-4ae478c79783";
  ad4.ad_content.type = AdContent::AdType::kAdNotification;
  ad4.ad_content.creative_instance_id = "47c73793-d1c1-4fdb-8530-4ae478c79783";
  ad4.ad_content.ad_action = ConfirmationType::kDownvoted;

  AdHistory ad5;  // Unsupported
  ad5.ad_content.uuid = "b7e1314c-73b0-4291-9cdd-6c5d2374c28f";
  ad5.ad_content.type = AdContent::AdType::kAdNotification;
  ad5.ad_content.creative_instance_id = "b7e1314c-73b0-4291-9cdd-6c5d2374c28f";
  ad5.ad_content.ad_action = ConfirmationType::kConversion;

  AdHistory ad6;  // Ad 1 (Viewed)
  ad6.ad_content.uuid = "ab9deba5-01bf-492b-9bb8-7bc4318fe272";
  ad5.ad_content.type = AdContent::AdType::kAdNotification;
  ad6.ad_content.creative_instance_id = "ab9deba5-01bf-492b-9bb8-7bc4318fe272";
  ad6.ad_content.ad_action = ConfirmationType::kViewed;

  AdHistory ad7;  // Unsupported
  ad7.ad_content.uuid = "5da2f2b3-85ca-4ba3-b879-634c5da9bdc6";
  ad7.ad_content.type = AdContent::AdType::kAdNotification;
  ad7.ad_content.creative_instance_id = "5da2f2b3-85ca-4ba3-b879-634c5da9bdc6";
  ad7.ad_content.ad_action = ConfirmationType::kDismissed;

  AdHistory ad8;  // Ad 1 (Clicked)
  ad8.ad_content.uuid = "ab9deba5-01bf-492b-9bb8-7bc4318fe272";
  ad8.ad_content.type = AdContent::AdType::kAdNotification;
  ad8.ad_content.creative_instance_id = "ab9deba5-01bf-492b-9bb8-7bc4318fe272";
  ad8.ad_content.ad_action = ConfirmationType::kClicked;

  std::deque<AdHistory> history = {
    ad1,
    ad2,
    ad3,
    ad4,
    ad5,
    ad6,
    ad7,
    ad8
  };

  // Act
  AdsHistoryConversionFilter filter;
  history = filter.Apply(history);

  // Assert
  const std::deque<AdHistory> expected_history = {
    ad6,  // Ad 1 (Viewed)
    ad8   // Ad 1 (Clicked)
  };

  EXPECT_EQ(expected_history, history);
}

}  // namespace ads
