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
  AdHistory ad1;
  ad1.parent_uuid = "69b684d7-d893-4f4e-b156-859919a0fcc9";
  ad1.ad_content.ad_action = ConfirmationType::kLanded;       // Unsupported

  AdHistory ad2;
  ad2.parent_uuid = "d3be2e79-ffa8-4b4e-b61e-88545055fbad";
  ad2.ad_content.ad_action = ConfirmationType::kFlagged;      // Unsupported

  AdHistory ad3;
  ad3.parent_uuid = "9390f66a-d4f2-4c8a-8315-1baed4aae612";
  ad3.ad_content.ad_action = ConfirmationType::kUpvoted;      // Unsupported

  AdHistory ad4;
  ad4.parent_uuid = "47c73793-d1c1-4fdb-8530-4ae478c79783";
  ad4.ad_content.ad_action = ConfirmationType::kDownvoted;    // Unsupported

  AdHistory ad5;
  ad5.parent_uuid = "b7e1314c-73b0-4291-9cdd-6c5d2374c28f";
  ad5.ad_content.ad_action = ConfirmationType::kConversion;   // Unsupported

  AdHistory ad6;
  ad6.parent_uuid = "ab9deba5-01bf-492b-9bb8-7bc4318fe272";
  ad6.ad_content.ad_action = ConfirmationType::kViewed;       // Ad 1 (Viewed)

  AdHistory ad7;
  ad7.parent_uuid = "5da2f2b3-85ca-4ba3-b879-634c5da9bdc6";
  ad7.ad_content.ad_action = ConfirmationType::kDismissed;    // Unsupported

  AdHistory ad8;
  ad8.parent_uuid = "ab9deba5-01bf-492b-9bb8-7bc4318fe272";
  ad8.ad_content.ad_action = ConfirmationType::kClicked;      // Ad 1 (Clicked)

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
