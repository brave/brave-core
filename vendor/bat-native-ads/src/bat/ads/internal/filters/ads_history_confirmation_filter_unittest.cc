/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/filters/ads_history_confirmation_filter.h"

#include <deque>

#include "testing/gtest/include/gtest/gtest.h"
#include "bat/ads/ad_history.h"
#include "bat/ads/internal/container_util.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

TEST(BatAdsHistoryConfirmationFilterTest,
    FilterActions) {
  // Arrange
  AdHistory ad1;
  ad1.parent_uuid = "ab9deba5-01bf-492b-9bb8-7bc4318fe272";  // Ad 1 (Viewed)
  ad1.ad_content.ad_action = ConfirmationType::kViewed;

  AdHistory ad2;
  ad2.parent_uuid = "a577e7fe-d86c-4997-bbaa-4041dfd4075c";  // Ad 2 (Viewed)
  ad2.ad_content.ad_action = ConfirmationType::kViewed;

  AdHistory ad3;
  ad3.parent_uuid = "ab9deba5-01bf-492b-9bb8-7bc4318fe272";  // Ad 1 (Clicked)
  ad3.ad_content.ad_action = ConfirmationType::kClicked;

  AdHistory ad4;
  ad4.parent_uuid = "4424ff92-fa91-4ca9-a651-96b59cf1f68b";  // Ad 3 (Dismissed)
  ad4.ad_content.ad_action = ConfirmationType::kDismissed;

  AdHistory ad5;
  ad5.parent_uuid = "4424ff92-fa91-4ca9-a651-96b59cf1f68b";  // Ad 3 (Viewed)
  ad5.ad_content.ad_action = ConfirmationType::kViewed;

  AdHistory ad6;
  ad6.parent_uuid = "d9253022-b023-4414-a85d-96b78d36435d";  // Ad 4 (Viewed)
  ad6.ad_content.ad_action = ConfirmationType::kViewed;

  std::deque<AdHistory> history = {
    ad1,
    ad2,
    ad3,
    ad4,
    ad5,
    ad6
  };

  // Act
  AdsHistoryConfirmationFilter filter;
  history = filter.Apply(history);

  // Assert
  const std::deque<AdHistory> expected_history = {
    ad2,  // Ad 2
    ad3,  // Ad 1 (Click) which should supersede Ad 1 (View)
    ad4,  // Ad 3 (Dismiss) which should supersede Ad 3 (View)
    ad6   // Ad 4
  };

  EXPECT_TRUE(CompareAsSets(expected_history, history));
}

TEST(BatAdsHistoryConfirmationFilterTest,
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
  ad6.parent_uuid = "ab9deba5-01bf-492b-9bb8-7bc4318fe272";   // Ad 1 (View)
  ad6.ad_content.ad_action = ConfirmationType::kViewed;

  std::deque<AdHistory> history = {
    ad1,
    ad2,
    ad3,
    ad4,
    ad5,
    ad6
  };

  // Act
  AdsHistoryConfirmationFilter filter;
  history = filter.Apply(history);

  // Assert
  const std::deque<AdHistory> expected_history = {
    ad6  // Ad 1 (View)
  };

  EXPECT_TRUE(CompareAsSets(expected_history, history));
}

}  // namespace ads
