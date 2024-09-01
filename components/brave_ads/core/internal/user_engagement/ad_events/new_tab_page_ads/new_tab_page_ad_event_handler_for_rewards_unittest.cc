/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/test/mock_callback.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_test_constants.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_test_util.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ads_database_util.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/new_tab_page_ad_builder.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_test_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/new_tab_page_ads/new_tab_page_ad_event_handler.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/new_tab_page_ads/new_tab_page_ad_event_handler_delegate_mock.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-shared.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/public/ad_units/new_tab_page_ad/new_tab_page_ad_info.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

NewTabPageAdInfo BuildAndSaveAd() {
  const CreativeNewTabPageAdInfo creative_ad =
      test::BuildCreativeNewTabPageAd(/*should_generate_random_uuids=*/false);
  database::SaveCreativeNewTabPageAds({creative_ad});
  return BuildNewTabPageAd(creative_ad);
}

}  // namespace

class BraveAdsNewTabPageAdEventHandlerForRewardsTest : public test::TestBase {
 protected:
  void SetUp() override {
    test::TestBase::SetUp();

    event_handler_.SetDelegate(&delegate_mock_);
  }

  void FireEventAndVerifyExpectations(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      const mojom::NewTabPageAdEventType mojom_ad_event_type,
      const bool should_fire_event) {
    base::MockCallback<FireNewTabPageAdEventHandlerCallback> callback;
    EXPECT_CALL(callback, Run(/*success=*/should_fire_event, placement_id,
                              mojom_ad_event_type));
    event_handler_.FireEvent(placement_id, creative_instance_id,
                             mojom_ad_event_type, callback.Get());
  }

  NewTabPageAdEventHandler event_handler_;
  ::testing::StrictMock<NewTabPageAdEventHandlerDelegateMock> delegate_mock_;
};

TEST_F(BraveAdsNewTabPageAdEventHandlerForRewardsTest, FireServedEvent) {
  // Arrange
  const NewTabPageAdInfo ad = BuildAndSaveAd();

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnDidFireNewTabPageAdServedEvent(ad));
  FireEventAndVerifyExpectations(
      ad.placement_id, ad.creative_instance_id,
      mojom::NewTabPageAdEventType::kServedImpression,
      /*should_fire_event=*/true);
}

TEST_F(BraveAdsNewTabPageAdEventHandlerForRewardsTest, FireViewedEvent) {
  // Arrange
  const NewTabPageAdInfo ad = BuildAndSaveAd();

  test::RecordAdEvent(ad, ConfirmationType::kServedImpression);

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnDidFireNewTabPageAdViewedEvent(ad));
  FireEventAndVerifyExpectations(
      ad.placement_id, ad.creative_instance_id,
      mojom::NewTabPageAdEventType::kViewedImpression,
      /*should_fire_event=*/true);
}

TEST_F(BraveAdsNewTabPageAdEventHandlerForRewardsTest,
       DoNotFireViewedEventIfAdPlacementWasAlreadyViewed) {
  // Arrange
  const NewTabPageAdInfo ad = BuildAndSaveAd();

  test::RecordAdEvents(ad, {ConfirmationType::kServedImpression,
                            ConfirmationType::kViewedImpression});

  // Act & Assert
  EXPECT_CALL(delegate_mock_,
              OnFailedToFireNewTabPageAdEvent(
                  ad.placement_id, ad.creative_instance_id,
                  mojom::NewTabPageAdEventType::kViewedImpression));
  FireEventAndVerifyExpectations(
      ad.placement_id, ad.creative_instance_id,
      mojom::NewTabPageAdEventType::kViewedImpression,
      /*should_fire_event=*/false);
}

TEST_F(BraveAdsNewTabPageAdEventHandlerForRewardsTest,
       DoNotFireViewedEventIfAdPlacementWasNotServed) {
  // Arrange
  const NewTabPageAdInfo ad = BuildAndSaveAd();

  // Act & Assert
  EXPECT_CALL(delegate_mock_,
              OnFailedToFireNewTabPageAdEvent(
                  ad.placement_id, ad.creative_instance_id,
                  mojom::NewTabPageAdEventType::kViewedImpression));
  FireEventAndVerifyExpectations(
      ad.placement_id, ad.creative_instance_id,
      mojom::NewTabPageAdEventType::kViewedImpression,
      /*should_fire_event=*/false);
}

TEST_F(BraveAdsNewTabPageAdEventHandlerForRewardsTest, FireClickedEvent) {
  // Arrange
  const NewTabPageAdInfo ad = BuildAndSaveAd();

  test::RecordAdEvents(ad, {ConfirmationType::kServedImpression,
                            ConfirmationType::kViewedImpression});

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnDidFireNewTabPageAdClickedEvent(ad));
  FireEventAndVerifyExpectations(ad.placement_id, ad.creative_instance_id,
                                 mojom::NewTabPageAdEventType::kClicked,
                                 /*should_fire_event=*/true);
}

TEST_F(BraveAdsNewTabPageAdEventHandlerForRewardsTest,
       DoNotFireClickedEventIfAdPlacementWasAlreadyClicked) {
  // Arrange
  const NewTabPageAdInfo ad = BuildAndSaveAd();

  test::RecordAdEvents(
      ad, {ConfirmationType::kServedImpression,
           ConfirmationType::kViewedImpression, ConfirmationType::kClicked});

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnFailedToFireNewTabPageAdEvent(
                                  ad.placement_id, ad.creative_instance_id,
                                  mojom::NewTabPageAdEventType::kClicked));
  FireEventAndVerifyExpectations(ad.placement_id, ad.creative_instance_id,
                                 mojom::NewTabPageAdEventType::kClicked,
                                 /*should_fire_event=*/false);
}

TEST_F(BraveAdsNewTabPageAdEventHandlerForRewardsTest,
       DoNotFireClickedEventIfAdPlacementWasNotServed) {
  // Arrange
  const NewTabPageAdInfo ad = BuildAndSaveAd();

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnFailedToFireNewTabPageAdEvent(
                                  ad.placement_id, ad.creative_instance_id,
                                  mojom::NewTabPageAdEventType::kClicked));
  FireEventAndVerifyExpectations(ad.placement_id, ad.creative_instance_id,
                                 mojom::NewTabPageAdEventType::kClicked,
                                 /*should_fire_event=*/false);
}

TEST_F(BraveAdsNewTabPageAdEventHandlerForRewardsTest,
       DoNotFireEventWithInvalidPlacementId) {
  // Act & Assert
  EXPECT_CALL(delegate_mock_,
              OnFailedToFireNewTabPageAdEvent(
                  test::kInvalidPlacementId, test::kCreativeInstanceId,
                  mojom::NewTabPageAdEventType::kServedImpression));
  FireEventAndVerifyExpectations(
      test::kInvalidPlacementId, test::kCreativeInstanceId,
      mojom::NewTabPageAdEventType::kServedImpression,
      /*should_fire_event=*/false);
}

TEST_F(BraveAdsNewTabPageAdEventHandlerForRewardsTest,
       DoNotFireEventWithInvalidCreativeInstanceId) {
  // Act & Assert
  EXPECT_CALL(delegate_mock_,
              OnFailedToFireNewTabPageAdEvent(
                  test::kPlacementId, test::kInvalidCreativeInstanceId,
                  mojom::NewTabPageAdEventType::kServedImpression));
  FireEventAndVerifyExpectations(
      test::kPlacementId, test::kInvalidCreativeInstanceId,
      mojom::NewTabPageAdEventType::kServedImpression,
      /*should_fire_event=*/false);
}

TEST_F(BraveAdsNewTabPageAdEventHandlerForRewardsTest,
       DoNotFireEventForMissingCreativeInstanceId) {
  // Arrange
  const NewTabPageAdInfo ad = BuildAndSaveAd();

  // Act & Assert
  EXPECT_CALL(delegate_mock_,
              OnFailedToFireNewTabPageAdEvent(
                  ad.placement_id, test::kMissingCreativeInstanceId,
                  mojom::NewTabPageAdEventType::kServedImpression));
  FireEventAndVerifyExpectations(
      ad.placement_id, test::kMissingCreativeInstanceId,
      mojom::NewTabPageAdEventType::kServedImpression,
      /*should_fire_event=*/false);
}

}  // namespace brave_ads
