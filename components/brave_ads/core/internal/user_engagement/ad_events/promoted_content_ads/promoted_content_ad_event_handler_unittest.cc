/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/promoted_content_ads/promoted_content_ad_event_handler.h"

#include "base/test/mock_callback.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_test_constants.h"
#include "brave/components/brave_ads/core/internal/ad_units/promoted_content_ad/promoted_content_ad_feature.h"
#include "brave/components/brave_ads/core/internal/ad_units/promoted_content_ad/promoted_content_ad_info.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/creatives/promoted_content_ads/creative_promoted_content_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/promoted_content_ads/creative_promoted_content_ad_test_util.h"
#include "brave/components/brave_ads/core/internal/creatives/promoted_content_ads/creative_promoted_content_ads_database_util.h"
#include "brave/components/brave_ads/core/internal/creatives/promoted_content_ads/promoted_content_ad_builder.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/permission_rules_test_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_test_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/promoted_content_ads/promoted_content_ad_event_handler_delegate.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/promoted_content_ads/promoted_content_ad_event_handler_delegate_mock.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

PromotedContentAdInfo BuildAndSaveAd() {
  const CreativePromotedContentAdInfo creative_ad =
      test::BuildCreativePromotedContentAd(
          /*should_generate_random_uuids=*/false);
  database::SaveCreativePromotedContentAds({creative_ad});
  return BuildPromotedContentAd(creative_ad);
}

}  // namespace

class BraveAdsPromotedContentAdEventHandlerTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    event_handler_.SetDelegate(&delegate_mock_);

    test::ForcePermissionRules();
  }

  void FireEventAndVerifyExpectations(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      const mojom::PromotedContentAdEventType event_type,
      const bool should_fire_event) {
    base::MockCallback<FirePromotedContentAdEventHandlerCallback> callback;
    EXPECT_CALL(callback,
                Run(/*success=*/should_fire_event, placement_id, event_type));
    event_handler_.FireEvent(placement_id, creative_instance_id, event_type,
                             callback.Get());
  }

  PromotedContentAdEventHandler event_handler_;
  ::testing::StrictMock<PromotedContentAdEventHandlerDelegateMock>
      delegate_mock_;
};

TEST_F(BraveAdsPromotedContentAdEventHandlerTest, FireViewedEvent) {
  // Arrange
  const PromotedContentAdInfo ad = BuildAndSaveAd();

  test::RecordAdEvent(ad, ConfirmationType::kServedImpression);

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnDidFirePromotedContentAdViewedEvent(ad));
  FireEventAndVerifyExpectations(
      ad.placement_id, ad.creative_instance_id,
      mojom::PromotedContentAdEventType::kViewedImpression,
      /*should_fire_event=*/true);
}

TEST_F(BraveAdsPromotedContentAdEventHandlerTest,
       DoNotFireViewedEventIfAdPlacementWasAlreadyViewed) {
  // Arrange
  const PromotedContentAdInfo ad = BuildAndSaveAd();

  test::RecordAdEvents(ad, {ConfirmationType::kServedImpression,
                            ConfirmationType::kViewedImpression});

  // Act & Assert
  EXPECT_CALL(delegate_mock_,
              OnFailedToFirePromotedContentAdEvent(
                  ad.placement_id, ad.creative_instance_id,
                  mojom::PromotedContentAdEventType::kViewedImpression));
  FireEventAndVerifyExpectations(
      ad.placement_id, ad.creative_instance_id,
      mojom::PromotedContentAdEventType::kViewedImpression,
      /*should_fire_event=*/false);
}

TEST_F(BraveAdsPromotedContentAdEventHandlerTest,
       DoNotFireViewedEventIfAdPlacementWasNotServed) {
  // Arrange
  const PromotedContentAdInfo ad = BuildAndSaveAd();

  // Act & Assert
  EXPECT_CALL(delegate_mock_,
              OnFailedToFirePromotedContentAdEvent(
                  ad.placement_id, ad.creative_instance_id,
                  mojom::PromotedContentAdEventType::kViewedImpression));
  FireEventAndVerifyExpectations(
      ad.placement_id, ad.creative_instance_id,
      mojom::PromotedContentAdEventType::kViewedImpression,
      /*should_fire_event=*/false);
}

TEST_F(BraveAdsPromotedContentAdEventHandlerTest, FireClickedEvent) {
  // Arrange
  const PromotedContentAdInfo ad = BuildAndSaveAd();

  test::RecordAdEvents(ad, {ConfirmationType::kServedImpression,
                            ConfirmationType::kViewedImpression});

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnDidFirePromotedContentAdClickedEvent(ad));
  FireEventAndVerifyExpectations(ad.placement_id, ad.creative_instance_id,
                                 mojom::PromotedContentAdEventType::kClicked,
                                 /*should_fire_event=*/true);
}

TEST_F(BraveAdsPromotedContentAdEventHandlerTest,
       DoNotFireClickedEventIfAdPlacementWasAlreadyClicked) {
  // Arrange
  const PromotedContentAdInfo ad = BuildAndSaveAd();

  test::RecordAdEvents(
      ad, {ConfirmationType::kServedImpression,
           ConfirmationType::kViewedImpression, ConfirmationType::kClicked});

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnFailedToFirePromotedContentAdEvent(
                                  ad.placement_id, ad.creative_instance_id,
                                  mojom::PromotedContentAdEventType::kClicked));
  FireEventAndVerifyExpectations(ad.placement_id, ad.creative_instance_id,
                                 mojom::PromotedContentAdEventType::kClicked,
                                 /*should_fire_event=*/false);
}

TEST_F(BraveAdsPromotedContentAdEventHandlerTest,
       DoNotFireClickedEventIfAdPlacementWasNotServed) {
  // Arrange
  const PromotedContentAdInfo ad = BuildAndSaveAd();

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnFailedToFirePromotedContentAdEvent(
                                  ad.placement_id, ad.creative_instance_id,
                                  mojom::PromotedContentAdEventType::kClicked));
  FireEventAndVerifyExpectations(ad.placement_id, ad.creative_instance_id,
                                 mojom::PromotedContentAdEventType::kClicked,
                                 /*should_fire_event=*/false);
}

TEST_F(BraveAdsPromotedContentAdEventHandlerTest,
       DoNotFireEventWithInvalidPlacementId) {
  // Act & Assert
  EXPECT_CALL(delegate_mock_,
              OnFailedToFirePromotedContentAdEvent(
                  test::kInvalidPlacementId, test::kCreativeInstanceId,
                  mojom::PromotedContentAdEventType::kServedImpression));
  FireEventAndVerifyExpectations(
      test::kInvalidPlacementId, test::kCreativeInstanceId,
      mojom::PromotedContentAdEventType::kServedImpression,
      /*should_fire_event=*/false);
}

TEST_F(BraveAdsPromotedContentAdEventHandlerTest,
       DoNotFireEventWithInvalidCreativeInstanceId) {
  // Act & Assert
  EXPECT_CALL(delegate_mock_,
              OnFailedToFirePromotedContentAdEvent(
                  test::kPlacementId, test::kInvalidCreativeInstanceId,
                  mojom::PromotedContentAdEventType::kServedImpression));
  FireEventAndVerifyExpectations(
      test::kPlacementId, test::kInvalidCreativeInstanceId,
      mojom::PromotedContentAdEventType::kServedImpression,
      /*should_fire_event=*/false);
}

TEST_F(BraveAdsPromotedContentAdEventHandlerTest,
       DoNotFireEventForMissingCreativeInstanceId) {
  // Arrange
  const PromotedContentAdInfo ad = BuildAndSaveAd();

  // Act & Assert
  EXPECT_CALL(delegate_mock_,
              OnFailedToFirePromotedContentAdEvent(
                  ad.placement_id, test::kMissingCreativeInstanceId,
                  mojom::PromotedContentAdEventType::kServedImpression));
  FireEventAndVerifyExpectations(
      ad.placement_id, test::kMissingCreativeInstanceId,
      mojom::PromotedContentAdEventType::kServedImpression,
      /*should_fire_event=*/false);
}

TEST_F(BraveAdsPromotedContentAdEventHandlerTest,
       FireEventIfNotExceededAdsPerHourCap) {
  // Arrange
  const PromotedContentAdInfo ad = BuildAndSaveAd();

  test::RecordAdEvents(ad, ConfirmationType::kServedImpression,
                       kMaximumPromotedContentAdsPerHour.Get() - 1);

  AdvanceClockBy(base::Hours(1) - base::Milliseconds(1));

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnDidFirePromotedContentAdServedEvent(ad));
  FireEventAndVerifyExpectations(
      ad.placement_id, ad.creative_instance_id,
      mojom::PromotedContentAdEventType::kServedImpression,
      /*should_fire_event=*/true);
}

TEST_F(BraveAdsPromotedContentAdEventHandlerTest,
       DoNotFireEventIfExceededAdsPerHourCap) {
  // Arrange
  const PromotedContentAdInfo ad = BuildAndSaveAd();

  test::RecordAdEvents(ad, ConfirmationType::kServedImpression,
                       kMaximumPromotedContentAdsPerHour.Get());

  AdvanceClockBy(base::Hours(1) - base::Milliseconds(1));

  // Act & Assert
  EXPECT_CALL(delegate_mock_,
              OnFailedToFirePromotedContentAdEvent(
                  ad.placement_id, ad.creative_instance_id,
                  mojom::PromotedContentAdEventType::kServedImpression));
  FireEventAndVerifyExpectations(
      ad.placement_id, ad.creative_instance_id,
      mojom::PromotedContentAdEventType::kServedImpression,
      /*should_fire_event=*/false);
}

TEST_F(BraveAdsPromotedContentAdEventHandlerTest,
       FireEventIfNotExceededAdsPerDayCap) {
  // Arrange
  const PromotedContentAdInfo ad = BuildAndSaveAd();

  test::RecordAdEvents(ad, ConfirmationType::kServedImpression,
                       kMaximumPromotedContentAdsPerDay.Get() - 1);

  AdvanceClockBy(base::Days(1) - base::Milliseconds(1));

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnDidFirePromotedContentAdServedEvent(ad));
  FireEventAndVerifyExpectations(
      ad.placement_id, ad.creative_instance_id,
      mojom::PromotedContentAdEventType::kServedImpression,
      /*should_fire_event=*/true);
}

TEST_F(BraveAdsPromotedContentAdEventHandlerTest,
       DoNotFireEventIfExceededAdsPerDayCap) {
  // Arrange
  const PromotedContentAdInfo ad = BuildAndSaveAd();

  test::RecordAdEvents(ad, ConfirmationType::kServedImpression,
                       kMaximumPromotedContentAdsPerDay.Get());

  AdvanceClockBy(base::Days(1) - base::Milliseconds(1));

  // Act & Assert
  EXPECT_CALL(delegate_mock_,
              OnFailedToFirePromotedContentAdEvent(
                  ad.placement_id, ad.creative_instance_id,
                  mojom::PromotedContentAdEventType::kServedImpression));
  FireEventAndVerifyExpectations(
      ad.placement_id, ad.creative_instance_id,
      mojom::PromotedContentAdEventType::kServedImpression,
      /*should_fire_event=*/false);
}

}  // namespace brave_ads
