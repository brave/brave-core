/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user/user_interaction/ad_events/inline_content_ads/inline_content_ad_event_handler.h"

#include <vector>

#include "base/test/mock_callback.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/creative_inline_content_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/creative_inline_content_ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/creative_inline_content_ads_database_util.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/inline_content_ad_builder.h"
#include "brave/components/brave_ads/core/internal/units/ad_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/user/user_interaction/ad_events/inline_content_ads/inline_content_ad_event_handler_delegate_mock.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-shared.h"
#include "brave/components/brave_ads/core/public/units/inline_content_ad/inline_content_ad_info.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

InlineContentAdInfo BuildAndSaveAd() {
  const CreativeInlineContentAdInfo creative_ad =
      test::BuildCreativeInlineContentAd(/*should_use_random_uuids=*/false);
  database::SaveCreativeInlineContentAds({creative_ad});
  return BuildInlineContentAd(creative_ad);
}

}  // namespace
class BraveAdsInlineContentAdEventHandlerTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    event_handler_.SetDelegate(&delegate_mock_);
  }

  void FireEvent(const std::string& placement_id,
                 const std::string& creative_instance_id,
                 const mojom::InlineContentAdEventType& event_type,
                 const bool should_fire_event) {
    base::MockCallback<FireInlineContentAdEventHandlerCallback> callback;
    EXPECT_CALL(callback,
                Run(/*success=*/should_fire_event, placement_id, event_type));
    event_handler_.FireEvent(placement_id, creative_instance_id, event_type,
                             callback.Get());
  }

  void FireEvents(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      const std::vector<mojom::InlineContentAdEventType>& event_types,
      const bool should_fire_event) {
    for (const auto& event_type : event_types) {
      FireEvent(placement_id, creative_instance_id, event_type,
                should_fire_event);
    }
  }

  InlineContentAdEventHandler event_handler_;
  ::testing::StrictMock<InlineContentAdEventHandlerDelegateMock> delegate_mock_;
};

TEST_F(BraveAdsInlineContentAdEventHandlerTest, FireServedEvent) {
  // Arrange
  const InlineContentAdInfo ad = BuildAndSaveAd();

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnDidFireInlineContentAdServedEvent(ad));

  FireEvent(ad.placement_id, ad.creative_instance_id,
            mojom::InlineContentAdEventType::kServed,
            /*should_fire_event=*/true);
}

TEST_F(BraveAdsInlineContentAdEventHandlerTest, FireViewedEvent) {
  // Arrange
  const InlineContentAdInfo ad = BuildAndSaveAd();

  EXPECT_CALL(delegate_mock_, OnDidFireInlineContentAdServedEvent(ad));

  FireEvent(ad.placement_id, ad.creative_instance_id,
            mojom::InlineContentAdEventType::kServed,
            /*should_fire_event=*/true);

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnDidFireInlineContentAdViewedEvent(ad));

  FireEvent(ad.placement_id, ad.creative_instance_id,
            mojom::InlineContentAdEventType::kViewed,
            /*should_fire_event=*/true);
}

TEST_F(BraveAdsInlineContentAdEventHandlerTest,
       DoNotFireViewedEventIfAdPlacementWasAlreadyViewed) {
  // Arrange
  const InlineContentAdInfo ad = BuildAndSaveAd();

  EXPECT_CALL(delegate_mock_, OnDidFireInlineContentAdServedEvent(ad));
  EXPECT_CALL(delegate_mock_, OnDidFireInlineContentAdViewedEvent(ad));

  FireEvents(ad.placement_id, ad.creative_instance_id,
             {mojom::InlineContentAdEventType::kServed,
              mojom::InlineContentAdEventType::kViewed},
             /*should_fire_event=*/true);

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnFailedToFireInlineContentAdEvent(
                                  ad.placement_id, ad.creative_instance_id,
                                  mojom::InlineContentAdEventType::kViewed));

  FireEvent(ad.placement_id, ad.creative_instance_id,
            mojom::InlineContentAdEventType::kViewed,
            /*should_fire_event=*/false);
}

TEST_F(BraveAdsInlineContentAdEventHandlerTest,
       DoNotFireViewedEventIfAdPlacementWasNotServed) {
  // Arrange
  const InlineContentAdInfo ad = BuildAndSaveAd();

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnFailedToFireInlineContentAdEvent(
                                  ad.placement_id, ad.creative_instance_id,
                                  mojom::InlineContentAdEventType::kViewed));

  FireEvent(ad.placement_id, ad.creative_instance_id,
            mojom::InlineContentAdEventType::kViewed,
            /*should_fire_event=*/false);
}

TEST_F(BraveAdsInlineContentAdEventHandlerTest, FireClickedEvent) {
  // Arrange
  const InlineContentAdInfo ad = BuildAndSaveAd();

  EXPECT_CALL(delegate_mock_, OnDidFireInlineContentAdServedEvent(ad));
  EXPECT_CALL(delegate_mock_, OnDidFireInlineContentAdViewedEvent(ad));

  FireEvents(ad.placement_id, ad.creative_instance_id,
             {mojom::InlineContentAdEventType::kServed,
              mojom::InlineContentAdEventType::kViewed},
             /*should_fire_event=*/true);

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnDidFireInlineContentAdClickedEvent(ad));

  FireEvent(ad.placement_id, ad.creative_instance_id,
            mojom::InlineContentAdEventType::kClicked,
            /*should_fire_event=*/true);
}

TEST_F(BraveAdsInlineContentAdEventHandlerTest,
       DoNotFireClickedEventIfAdPlacementWasAlreadyClicked) {
  // Arrange
  const InlineContentAdInfo ad = BuildAndSaveAd();

  EXPECT_CALL(delegate_mock_, OnDidFireInlineContentAdServedEvent(ad));
  EXPECT_CALL(delegate_mock_, OnDidFireInlineContentAdViewedEvent(ad));
  EXPECT_CALL(delegate_mock_, OnDidFireInlineContentAdClickedEvent(ad));

  FireEvents(ad.placement_id, ad.creative_instance_id,
             {mojom::InlineContentAdEventType::kServed,
              mojom::InlineContentAdEventType::kViewed,
              mojom::InlineContentAdEventType::kClicked},
             /*should_fire_event=*/true);

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnFailedToFireInlineContentAdEvent(
                                  ad.placement_id, ad.creative_instance_id,
                                  mojom::InlineContentAdEventType::kClicked));

  FireEvent(ad.placement_id, ad.creative_instance_id,
            mojom::InlineContentAdEventType::kClicked,
            /*should_fire_event=*/false);
}

TEST_F(BraveAdsInlineContentAdEventHandlerTest,
       DoNotFireClickedEventIfAdPlacementWasNotServed) {
  // Arrange
  const InlineContentAdInfo ad = BuildAndSaveAd();

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnFailedToFireInlineContentAdEvent(
                                  ad.placement_id, ad.creative_instance_id,
                                  mojom::InlineContentAdEventType::kClicked));

  FireEvent(ad.placement_id, ad.creative_instance_id,
            mojom::InlineContentAdEventType::kClicked,
            /*should_fire_event=*/false);
}

TEST_F(BraveAdsInlineContentAdEventHandlerTest,
       DoNotFireEventWithInvalidPlacementId) {
  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnFailedToFireInlineContentAdEvent(
                                  kInvalidPlacementId, kCreativeInstanceId,
                                  mojom::InlineContentAdEventType::kServed));

  FireEvent(kInvalidPlacementId, kCreativeInstanceId,
            mojom::InlineContentAdEventType::kServed,
            /*should_fire_event=*/false);
}

TEST_F(BraveAdsInlineContentAdEventHandlerTest,
       DoNotFireEventWithInvalidCreativeInstanceId) {
  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnFailedToFireInlineContentAdEvent(
                                  kPlacementId, kInvalidCreativeInstanceId,
                                  mojom::InlineContentAdEventType::kServed));

  FireEvent(kPlacementId, kInvalidCreativeInstanceId,
            mojom::InlineContentAdEventType::kServed,
            /*should_fire_event=*/false);
}

TEST_F(BraveAdsInlineContentAdEventHandlerTest,
       DoNotFireEventForMissingCreativeInstanceId) {
  // Arrange
  const InlineContentAdInfo ad = BuildAndSaveAd();

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnFailedToFireInlineContentAdEvent(
                                  ad.placement_id, kMissingCreativeInstanceId,
                                  mojom::InlineContentAdEventType::kServed));

  FireEvent(ad.placement_id, kMissingCreativeInstanceId,
            mojom::InlineContentAdEventType::kServed,
            /*should_fire_event=*/false);
}

}  // namespace brave_ads
