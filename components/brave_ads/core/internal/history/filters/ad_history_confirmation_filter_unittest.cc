/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/history/filters/ad_history_confirmation_filter.h"

#include "brave/components/brave_ads/core/public/history/ad_history_item_info.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsAdHistoryConfirmationFilterTest, FilterActions) {
  // Arrange
  AdHistoryItemInfo ad_history_item_1;  // Ad 1 (Viewed impression)
  ad_history_item_1.placement_id = "b7a0aa61-7c3a-40f8-aa29-d416b64cebd9";
  ad_history_item_1.type = AdType::kNotificationAd;
  ad_history_item_1.creative_instance_id =
      "ab9deba5-01bf-492b-9bb8-7bc4318fe272";
  ad_history_item_1.confirmation_type = ConfirmationType::kViewedImpression;

  AdHistoryItemInfo ad_history_item_2;  // Ad 2 (Viewed impression)
  ad_history_item_2.placement_id = "137c7cc0-7923-428a-8598-faee87159d99";
  ad_history_item_2.type = AdType::kNotificationAd;
  ad_history_item_2.creative_instance_id =
      "a577e7fe-d86c-4997-bbaa-4041dfd4075c";
  ad_history_item_2.confirmation_type = ConfirmationType::kViewedImpression;

  AdHistoryItemInfo ad_history_item_3;  // Ad 1 (Clicked)
  ad_history_item_3.placement_id = "b7a0aa61-7c3a-40f8-aa29-d416b64cebd9";
  ad_history_item_3.type = AdType::kNotificationAd;
  ad_history_item_3.creative_instance_id =
      "ab9deba5-01bf-492b-9bb8-7bc4318fe272";
  ad_history_item_3.confirmation_type = ConfirmationType::kClicked;

  AdHistoryItemInfo ad_history_item_4;  // Ad 3 (Dismissed)
  ad_history_item_4.placement_id = "fc5c8d59-ba66-443c-8721-f06161e73f23";
  ad_history_item_4.type = AdType::kNotificationAd;
  ad_history_item_4.creative_instance_id =
      "4424ff92-fa91-4ca9-a651-96b59cf1f68b";
  ad_history_item_4.confirmation_type = ConfirmationType::kDismissed;

  AdHistoryItemInfo ad_history_item_5;  // Ad 3 (Viewed impression)
  ad_history_item_5.placement_id = "fc5c8d59-ba66-443c-8721-f06161e73f23";
  ad_history_item_5.type = AdType::kNotificationAd;
  ad_history_item_5.creative_instance_id =
      "4424ff92-fa91-4ca9-a651-96b59cf1f68b";
  ad_history_item_5.confirmation_type = ConfirmationType::kViewedImpression;

  AdHistoryItemInfo ad_history_item_6;  // Ad 4 (Viewed impression)
  ad_history_item_6.placement_id = "6cbda0fa-5c00-4a49-985a-b76318b404c1";
  ad_history_item_6.type = AdType::kNotificationAd;
  ad_history_item_6.creative_instance_id =
      "d9253022-b023-4414-a85d-96b78d36435d";
  ad_history_item_6.confirmation_type = ConfirmationType::kViewedImpression;

  AdHistoryItemInfo ad_history_item_7;  // Ad 5 (Viewed impression)
  ad_history_item_7.placement_id = "09a30dc0-6645-4bda-ad30-f607e6f43306";
  ad_history_item_7.type = AdType::kNotificationAd;
  ad_history_item_7.creative_instance_id =
      "dc540882-6927-4e22-8597-aa80f339f0fd";
  ad_history_item_7.confirmation_type = ConfirmationType::kViewedImpression;

  AdHistoryList ad_history = {ad_history_item_1, ad_history_item_2,
                              ad_history_item_3, ad_history_item_4,
                              ad_history_item_5, ad_history_item_6,
                              ad_history_item_7};

  const AdHistoryConfirmationFilter filter;

  // Act
  filter.Apply(ad_history);

  // Assert
  const AdHistoryList expected_ad_history = {
      ad_history_item_2,  // Ad 2
      ad_history_item_3,  // Ad 1 (Click) which should supersede Ad 1 (View)
      ad_history_item_4,  // Ad 3 (Dismiss) which should supersede Ad 3 (View)
      ad_history_item_6,  // Ad 4
      ad_history_item_7   // Ad 5
  };
  EXPECT_THAT(expected_ad_history,
              ::testing::UnorderedElementsAreArray(ad_history));
}

TEST(BraveAdsAdHistoryConfirmationFilterTest, FilterUnsupportedActions) {
  // Arrange
  AdHistoryItemInfo ad_history_item_1;  // Unsupported
  ad_history_item_1.placement_id = "54ee85b3-b84e-4e80-a6db-8954b554f466";
  ad_history_item_1.type = AdType::kNotificationAd;
  ad_history_item_1.creative_instance_id =
      "69b684d7-d893-4f4e-b156-859919a0fcc9";
  ad_history_item_1.confirmation_type = ConfirmationType::kLanded;

  AdHistoryItemInfo ad_history_item_2;  // Unsupported
  ad_history_item_2.placement_id = "f067d4a9-0b92-4d3b-8cc5-e9baf89081c1";
  ad_history_item_2.type = AdType::kNewTabPageAd;
  ad_history_item_2.creative_instance_id =
      "d3be2e79-ffa8-4b4e-b61e-88545055fbad";
  ad_history_item_2.confirmation_type =
      ConfirmationType::kMarkAdAsInappropriate;

  AdHistoryItemInfo ad_history_item_3;  // Unsupported
  ad_history_item_3.placement_id = "445fae45-c9f5-4cfe-abfb-85e23c7bd1c7";
  ad_history_item_3.type = AdType::kNotificationAd;
  ad_history_item_3.creative_instance_id =
      "9390f66a-d4f2-4c8a-8315-1baed4aae612";
  ad_history_item_3.confirmation_type = ConfirmationType::kLikedAd;

  AdHistoryItemInfo ad_history_item_4;  // Unsupported
  ad_history_item_4.placement_id = "a86a11d7-674c-494e-844d-f62417c2357b";
  ad_history_item_4.type = AdType::kPromotedContentAd;
  ad_history_item_4.creative_instance_id =
      "47c73793-d1c1-4fdb-8530-4ae478c79783";
  ad_history_item_4.confirmation_type = ConfirmationType::kDislikedAd;

  AdHistoryItemInfo ad_history_item_5;  // Unsupported
  ad_history_item_5.placement_id = "fc82694e-b518-4fb0-84ca-5cb7a055416a";
  ad_history_item_5.type = AdType::kNotificationAd;
  ad_history_item_5.creative_instance_id =
      "b7e1314c-73b0-4291-9cdd-6c5d2374c28f";
  ad_history_item_5.confirmation_type = ConfirmationType::kConversion;

  AdHistoryItemInfo ad_history_item_6;  // View impression
  ad_history_item_6.placement_id = "5c476298-b912-49e1-b827-6096c5829d97";
  ad_history_item_6.type = AdType::kInlineContentAd;
  ad_history_item_6.creative_instance_id =
      "ab9deba5-01bf-492b-9bb8-7bc4318fe272";
  ad_history_item_6.confirmation_type = ConfirmationType::kViewedImpression;

  AdHistoryItemInfo ad_history_item_7;  // Dismiss
  ad_history_item_7.placement_id = "1ec4f1ba-4255-4ecf-8701-8e550744cdf8";
  ad_history_item_7.type = AdType::kSearchResultAd;
  ad_history_item_7.creative_instance_id =
      "d5d47c90-5c6b-4aa2-bd05-582ff6e4a03e";
  ad_history_item_7.confirmation_type = ConfirmationType::kDismissed;

  AdHistoryItemInfo ad_history_item_8;  // Click
  ad_history_item_8.placement_id = "ab9deba5-01bf-492b-9bb8-7bc4318fe272";
  ad_history_item_8.type = AdType::kNewTabPageAd;
  ad_history_item_8.creative_instance_id =
      "e00ccc4a-3186-4b56-9725-aeaf19095f96";
  ad_history_item_8.confirmation_type = ConfirmationType::kClicked;

  AdHistoryList ad_history = {ad_history_item_1, ad_history_item_2,
                              ad_history_item_3, ad_history_item_4,
                              ad_history_item_5, ad_history_item_6,
                              ad_history_item_7, ad_history_item_8};

  const AdHistoryConfirmationFilter filter;

  // Act
  filter.Apply(ad_history);

  // Assert
  const AdHistoryList expected_ad_history = {
      ad_history_item_6,  // View impression
      ad_history_item_7,  // Dismiss
      ad_history_item_8   // Click
  };
  EXPECT_THAT(expected_ad_history,
              ::testing::UnorderedElementsAreArray(ad_history));
}

}  // namespace brave_ads
