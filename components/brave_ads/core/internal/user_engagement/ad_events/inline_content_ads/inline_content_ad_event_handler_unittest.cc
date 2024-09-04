/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/inline_content_ads/inline_content_ad_event_handler.h"

#include "base/test/mock_callback.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_test_constants.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/creative_inline_content_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/creative_inline_content_ad_test_util.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/creative_inline_content_ads_database_util.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/inline_content_ad_builder.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_test_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/inline_content_ads/inline_content_ad_event_handler_delegate_mock.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/ad_units/inline_content_ad/inline_content_ad_info.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

InlineContentAdInfo BuildAndSaveAd() {
  const CreativeInlineContentAdInfo creative_ad =
      test::BuildCreativeInlineContentAd(
          /*should_generate_random_uuids=*/false);
  database::SaveCreativeInlineContentAds({creative_ad});
  return BuildInlineContentAd(creative_ad);
}

}  // namespace
class BraveAdsInlineContentAdEventHandlerTest : public test::TestBase {
 protected:
  void SetUp() override {
    test::TestBase::SetUp();

    event_handler_.SetDelegate(&delegate_mock_);
  }

  void FireEventAndVerifyExpectations(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      const mojom::InlineContentAdEventType mojom_ad_event_type,
      const bool should_fire_event) {
    base::MockCallback<FireInlineContentAdEventHandlerCallback> callback;
    EXPECT_CALL(callback, Run(/*success=*/should_fire_event, placement_id,
                              mojom_ad_event_type));
    event_handler_.FireEvent(placement_id, creative_instance_id,
                             mojom_ad_event_type, callback.Get());
  }

  InlineContentAdEventHandler event_handler_;
  ::testing::StrictMock<InlineContentAdEventHandlerDelegateMock> delegate_mock_;
};

TEST_F(BraveAdsInlineContentAdEventHandlerTest, FireServedEvent) {
  // Arrange
  const InlineContentAdInfo ad = BuildAndSaveAd();

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnDidFireInlineContentAdServedEvent(ad));
  FireEventAndVerifyExpectations(
      ad.placement_id, ad.creative_instance_id,
      mojom::InlineContentAdEventType::kServedImpression,
      /*should_fire_event=*/true);
}

TEST_F(BraveAdsInlineContentAdEventHandlerTest, FireViewedEvent) {
  // Arrange
  const InlineContentAdInfo ad = BuildAndSaveAd();
  test::RecordAdEvent(ad, mojom::ConfirmationType::kServedImpression);

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnDidFireInlineContentAdViewedEvent(ad));
  FireEventAndVerifyExpectations(
      ad.placement_id, ad.creative_instance_id,
      mojom::InlineContentAdEventType::kViewedImpression,
      /*should_fire_event=*/true);
}

TEST_F(BraveAdsInlineContentAdEventHandlerTest,
       DoNotFireViewedEventIfAdPlacementWasAlreadyViewed) {
  // Arrange
  const InlineContentAdInfo ad = BuildAndSaveAd();
  test::RecordAdEvents(ad, {mojom::ConfirmationType::kServedImpression,
                            mojom::ConfirmationType::kViewedImpression});

  // Act & Assert
  EXPECT_CALL(delegate_mock_,
              OnFailedToFireInlineContentAdEvent(
                  ad.placement_id, ad.creative_instance_id,
                  mojom::InlineContentAdEventType::kViewedImpression));
  FireEventAndVerifyExpectations(
      ad.placement_id, ad.creative_instance_id,
      mojom::InlineContentAdEventType::kViewedImpression,
      /*should_fire_event=*/false);
}

TEST_F(BraveAdsInlineContentAdEventHandlerTest,
       DoNotFireViewedEventIfAdPlacementWasNotServed) {
  // Arrange
  const InlineContentAdInfo ad = BuildAndSaveAd();

  // Act & Assert
  EXPECT_CALL(delegate_mock_,
              OnFailedToFireInlineContentAdEvent(
                  ad.placement_id, ad.creative_instance_id,
                  mojom::InlineContentAdEventType::kViewedImpression));
  FireEventAndVerifyExpectations(
      ad.placement_id, ad.creative_instance_id,
      mojom::InlineContentAdEventType::kViewedImpression,
      /*should_fire_event=*/false);
}

TEST_F(BraveAdsInlineContentAdEventHandlerTest, FireClickedEvent) {
  // Arrange
  const InlineContentAdInfo ad = BuildAndSaveAd();
  test::RecordAdEvents(ad, {mojom::ConfirmationType::kServedImpression,
                            mojom::ConfirmationType::kViewedImpression});

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnDidFireInlineContentAdClickedEvent(ad));
  FireEventAndVerifyExpectations(ad.placement_id, ad.creative_instance_id,
                                 mojom::InlineContentAdEventType::kClicked,
                                 /*should_fire_event=*/true);
}

TEST_F(BraveAdsInlineContentAdEventHandlerTest,
       DoNotFireClickedEventIfAdPlacementWasAlreadyClicked) {
  // Arrange
  const InlineContentAdInfo ad = BuildAndSaveAd();
  test::RecordAdEvents(ad, {mojom::ConfirmationType::kServedImpression,
                            mojom::ConfirmationType::kViewedImpression,
                            mojom::ConfirmationType::kClicked});

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnFailedToFireInlineContentAdEvent(
                                  ad.placement_id, ad.creative_instance_id,
                                  mojom::InlineContentAdEventType::kClicked));
  FireEventAndVerifyExpectations(ad.placement_id, ad.creative_instance_id,
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
  FireEventAndVerifyExpectations(ad.placement_id, ad.creative_instance_id,
                                 mojom::InlineContentAdEventType::kClicked,
                                 /*should_fire_event=*/false);
}

TEST_F(BraveAdsInlineContentAdEventHandlerTest,
       DoNotFireEventWithInvalidPlacementId) {
  // Act & Assert
  EXPECT_CALL(delegate_mock_,
              OnFailedToFireInlineContentAdEvent(
                  test::kInvalidPlacementId, test::kCreativeInstanceId,
                  mojom::InlineContentAdEventType::kServedImpression));
  FireEventAndVerifyExpectations(
      test::kInvalidPlacementId, test::kCreativeInstanceId,
      mojom::InlineContentAdEventType::kServedImpression,
      /*should_fire_event=*/false);
}

TEST_F(BraveAdsInlineContentAdEventHandlerTest,
       DoNotFireEventWithInvalidCreativeInstanceId) {
  // Act & Assert
  EXPECT_CALL(delegate_mock_,
              OnFailedToFireInlineContentAdEvent(
                  test::kPlacementId, test::kInvalidCreativeInstanceId,
                  mojom::InlineContentAdEventType::kServedImpression));
  FireEventAndVerifyExpectations(
      test::kPlacementId, test::kInvalidCreativeInstanceId,
      mojom::InlineContentAdEventType::kServedImpression,
      /*should_fire_event=*/false);
}

TEST_F(BraveAdsInlineContentAdEventHandlerTest,
       DoNotFireEventForMissingCreativeInstanceId) {
  // Arrange
  const InlineContentAdInfo ad = BuildAndSaveAd();

  // Act & Assert
  EXPECT_CALL(delegate_mock_,
              OnFailedToFireInlineContentAdEvent(
                  ad.placement_id, test::kMissingCreativeInstanceId,
                  mojom::InlineContentAdEventType::kServedImpression));
  FireEventAndVerifyExpectations(
      ad.placement_id, test::kMissingCreativeInstanceId,
      mojom::InlineContentAdEventType::kServedImpression,
      /*should_fire_event=*/false);
}

}  // namespace brave_ads
