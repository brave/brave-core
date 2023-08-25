/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/history/filters/confirmation_history_filter.h"

#include "base/containers/circular_deque.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_container_util.h"
#include "brave/components/brave_ads/core/public/history/history_item_info.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsConfirmationHistoryFilterTest, FilterActions) {
  // Arrange
  HistoryItemInfo ad1;  // Ad 1 (Viewed)
  ad1.ad_content.placement_id = "b7a0aa61-7c3a-40f8-aa29-d416b64cebd9";
  ad1.ad_content.type = AdType::kNotificationAd;
  ad1.ad_content.creative_instance_id = "ab9deba5-01bf-492b-9bb8-7bc4318fe272";
  ad1.ad_content.confirmation_type = ConfirmationType::kViewed;

  HistoryItemInfo ad2;  // Ad 2 (Viewed)
  ad2.ad_content.placement_id = "137c7cc0-7923-428a-8598-faee87159d99";
  ad2.ad_content.type = AdType::kNotificationAd;
  ad2.ad_content.creative_instance_id = "a577e7fe-d86c-4997-bbaa-4041dfd4075c";
  ad2.ad_content.confirmation_type = ConfirmationType::kViewed;

  HistoryItemInfo ad3;  // Ad 1 (Clicked)
  ad3.ad_content.placement_id = "b7a0aa61-7c3a-40f8-aa29-d416b64cebd9";
  ad3.ad_content.type = AdType::kNotificationAd;
  ad3.ad_content.creative_instance_id = "ab9deba5-01bf-492b-9bb8-7bc4318fe272";
  ad3.ad_content.confirmation_type = ConfirmationType::kClicked;

  HistoryItemInfo ad4;  // Ad 3 (Dismissed)
  ad4.ad_content.placement_id = "fc5c8d59-ba66-443c-8721-f06161e73f23";
  ad4.ad_content.type = AdType::kNotificationAd;
  ad4.ad_content.creative_instance_id = "4424ff92-fa91-4ca9-a651-96b59cf1f68b";
  ad4.ad_content.confirmation_type = ConfirmationType::kDismissed;

  HistoryItemInfo ad5;  // Ad 3 (Viewed)
  ad5.ad_content.placement_id = "fc5c8d59-ba66-443c-8721-f06161e73f23";
  ad5.ad_content.type = AdType::kNotificationAd;
  ad5.ad_content.creative_instance_id = "4424ff92-fa91-4ca9-a651-96b59cf1f68b";
  ad5.ad_content.confirmation_type = ConfirmationType::kViewed;

  HistoryItemInfo ad6;  // Ad 4 (Viewed)
  ad6.ad_content.placement_id = "6cbda0fa-5c00-4a49-985a-b76318b404c1";
  ad6.ad_content.type = AdType::kNotificationAd;
  ad6.ad_content.creative_instance_id = "d9253022-b023-4414-a85d-96b78d36435d";
  ad6.ad_content.confirmation_type = ConfirmationType::kViewed;

  HistoryItemInfo ad7;  // Ad 5 (Viewed)
  ad7.ad_content.placement_id = "09a30dc0-6645-4bda-ad30-f607e6f43306";
  ad7.ad_content.type = AdType::kNotificationAd;
  ad7.ad_content.creative_instance_id = "dc540882-6927-4e22-8597-aa80f339f0fd";
  ad7.ad_content.confirmation_type = ConfirmationType::kViewed;

  HistoryItemList history = {ad1, ad2, ad3, ad4, ad5, ad6, ad7};

  // Act
  const ConfirmationHistoryFilter filter;
  history = filter.Apply(history);

  // Assert
  const HistoryItemList expected_history = {
      ad2,  // Ad 2
      ad3,  // Ad 1 (Click) which should supersede Ad 1 (View)
      ad4,  // Ad 3 (Dismiss) which should supersede Ad 3 (View)
      ad6,  // Ad 4
      ad7   // Ad 5
  };

  EXPECT_TRUE(ContainersEq(expected_history, history));
}

TEST(BraveAdsConfirmationHistoryFilterTest, FilterUnsupportedActions) {
  // Arrange
  HistoryItemInfo ad1;  // Unsupported
  ad1.ad_content.placement_id = "54ee85b3-b84e-4e80-a6db-8954b554f466";
  ad1.ad_content.type = AdType::kNotificationAd;
  ad1.ad_content.creative_instance_id = "69b684d7-d893-4f4e-b156-859919a0fcc9";
  ad1.ad_content.confirmation_type = ConfirmationType::kTransferred;

  HistoryItemInfo ad2;  // Unsupported
  ad2.ad_content.placement_id = "f067d4a9-0b92-4d3b-8cc5-e9baf89081c1";
  ad2.ad_content.type = AdType::kNewTabPageAd;
  ad2.ad_content.creative_instance_id = "d3be2e79-ffa8-4b4e-b61e-88545055fbad";
  ad2.ad_content.confirmation_type = ConfirmationType::kFlagged;

  HistoryItemInfo ad3;  // Unsupported
  ad3.ad_content.placement_id = "445fae45-c9f5-4cfe-abfb-85e23c7bd1c7";
  ad3.ad_content.type = AdType::kNotificationAd;
  ad3.ad_content.creative_instance_id = "9390f66a-d4f2-4c8a-8315-1baed4aae612";
  ad3.ad_content.confirmation_type = ConfirmationType::kUpvoted;

  HistoryItemInfo ad4;  // Unsupported
  ad4.ad_content.placement_id = "a86a11d7-674c-494e-844d-f62417c2357b";
  ad4.ad_content.type = AdType::kPromotedContentAd;
  ad4.ad_content.creative_instance_id = "47c73793-d1c1-4fdb-8530-4ae478c79783";
  ad4.ad_content.confirmation_type = ConfirmationType::kDownvoted;

  HistoryItemInfo ad5;  // Unsupported
  ad5.ad_content.placement_id = "fc82694e-b518-4fb0-84ca-5cb7a055416a";
  ad5.ad_content.type = AdType::kNotificationAd;
  ad5.ad_content.creative_instance_id = "b7e1314c-73b0-4291-9cdd-6c5d2374c28f";
  ad5.ad_content.confirmation_type = ConfirmationType::kConversion;

  HistoryItemInfo ad6;  // View
  ad6.ad_content.placement_id = "5c476298-b912-49e1-b827-6096c5829d97";
  ad6.ad_content.type = AdType::kInlineContentAd;
  ad6.ad_content.creative_instance_id = "ab9deba5-01bf-492b-9bb8-7bc4318fe272";
  ad6.ad_content.confirmation_type = ConfirmationType::kViewed;

  HistoryItemInfo ad7;  // Dismiss
  ad7.ad_content.placement_id = "1ec4f1ba-4255-4ecf-8701-8e550744cdf8";
  ad7.ad_content.type = AdType::kSearchResultAd;
  ad7.ad_content.creative_instance_id = "d5d47c90-5c6b-4aa2-bd05-582ff6e4a03e";
  ad7.ad_content.confirmation_type = ConfirmationType::kDismissed;

  HistoryItemInfo ad8;  // Click
  ad8.ad_content.placement_id = "ab9deba5-01bf-492b-9bb8-7bc4318fe272";
  ad8.ad_content.type = AdType::kNewTabPageAd;
  ad8.ad_content.creative_instance_id = "e00ccc4a-3186-4b56-9725-aeaf19095f96";
  ad8.ad_content.confirmation_type = ConfirmationType::kClicked;

  HistoryItemList history = {ad1, ad2, ad3, ad4, ad5, ad6, ad7, ad8};

  // Act
  const ConfirmationHistoryFilter filter;
  history = filter.Apply(history);

  // Assert
  const HistoryItemList expected_history = {
      ad6,  // View
      ad7,  // Dismiss
      ad8   // Click
  };

  EXPECT_TRUE(ContainersEq(expected_history, history));
}

}  // namespace brave_ads
