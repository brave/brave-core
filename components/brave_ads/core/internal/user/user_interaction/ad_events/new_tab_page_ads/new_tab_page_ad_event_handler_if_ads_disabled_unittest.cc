/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user/user_interaction/ad_events/new_tab_page_ads/new_tab_page_ad_event_handler.h"

#include "base/test/mock_callback.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ads_database_util.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/new_tab_page_ad_builder.h"
#include "brave/components/brave_ads/core/internal/settings/settings_unittest_util.h"
#include "brave/components/brave_ads/core/internal/units/ad_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/user/user_interaction/ad_events/new_tab_page_ads/new_tab_page_ad_event_handler_delegate_mock.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-shared.h"
#include "brave/components/brave_ads/core/public/units/new_tab_page_ad/new_tab_page_ad_info.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

NewTabPageAdInfo BuildAndSaveAd() {
  const CreativeNewTabPageAdInfo creative_ad =
      test::BuildCreativeNewTabPageAd(/*should_use_random_uuids=*/true);
  database::SaveCreativeNewTabPageAds({creative_ad});
  return BuildNewTabPageAd(creative_ad);
}

}  // namespace

class BraveAdsNewTabPageAdEventHandlerIfUserHasNotJoinedBraveRewardsTest
    : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    test::DisableBraveRewards();

    event_handler_.SetDelegate(&delegate_mock_);
  }

  void FireEvent(const std::string& placement_id,
                 const std::string& creative_instance_id,
                 const mojom::NewTabPageAdEventType& event_type,
                 const bool should_fire_event) {
    base::MockCallback<FireNewTabPageAdEventHandlerCallback> callback;
    EXPECT_CALL(callback,
                Run(/*success=*/should_fire_event, placement_id, event_type));
    event_handler_.FireEvent(placement_id, creative_instance_id, event_type,
                             callback.Get());
  }

  void FireEvents(const std::string& placement_id,
                  const std::string& creative_instance_id,
                  const std::vector<mojom::NewTabPageAdEventType>& event_types,
                  const bool should_fire_event) {
    for (const auto& event_type : event_types) {
      FireEvent(placement_id, creative_instance_id, event_type,
                should_fire_event);
    }
  }

  NewTabPageAdEventHandler event_handler_;
  ::testing::StrictMock<NewTabPageAdEventHandlerDelegateMock> delegate_mock_;
};

TEST_F(BraveAdsNewTabPageAdEventHandlerIfUserHasNotJoinedBraveRewardsTest,
       FireServedEvent) {
  // Arrange
  const NewTabPageAdInfo ad = BuildAndSaveAd();

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnDidFireNewTabPageAdServedEvent(ad));

  FireEvent(ad.placement_id, ad.creative_instance_id,
            mojom::NewTabPageAdEventType::kServed,
            /*should_fire_event=*/true);
}

TEST_F(BraveAdsNewTabPageAdEventHandlerIfUserHasNotJoinedBraveRewardsTest,
       FireViewedEvent) {
  // Arrange
  const NewTabPageAdInfo ad = BuildAndSaveAd();

  EXPECT_CALL(delegate_mock_, OnDidFireNewTabPageAdServedEvent(ad));

  FireEvent(ad.placement_id, ad.creative_instance_id,
            mojom::NewTabPageAdEventType::kServed,
            /*should_fire_event=*/true);

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnDidFireNewTabPageAdViewedEvent(ad));

  FireEvent(ad.placement_id, ad.creative_instance_id,
            mojom::NewTabPageAdEventType::kViewed,
            /*should_fire_event=*/true);
}

TEST_F(BraveAdsNewTabPageAdEventHandlerIfUserHasNotJoinedBraveRewardsTest,
       DoNotFireViewedEventIfAdPlacementWasAlreadyViewed) {
  // Arrange
  const NewTabPageAdInfo ad = BuildAndSaveAd();

  EXPECT_CALL(delegate_mock_, OnDidFireNewTabPageAdServedEvent(ad));
  EXPECT_CALL(delegate_mock_, OnDidFireNewTabPageAdViewedEvent(ad));

  FireEvents(ad.placement_id, ad.creative_instance_id,
             {mojom::NewTabPageAdEventType::kServed,
              mojom::NewTabPageAdEventType::kViewed},
             /*should_fire_event=*/true);

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnFailedToFireNewTabPageAdEvent(
                                  ad.placement_id, ad.creative_instance_id,
                                  mojom::NewTabPageAdEventType::kViewed));

  FireEvent(ad.placement_id, ad.creative_instance_id,
            mojom::NewTabPageAdEventType::kViewed, /*should_fire_event=*/false);
}

TEST_F(BraveAdsNewTabPageAdEventHandlerIfUserHasNotJoinedBraveRewardsTest,
       DoNotFireViewedEventIfAdPlacementWasNotServed) {
  // Arrange
  const NewTabPageAdInfo ad = BuildAndSaveAd();

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnFailedToFireNewTabPageAdEvent(
                                  ad.placement_id, ad.creative_instance_id,
                                  mojom::NewTabPageAdEventType::kViewed));

  FireEvent(ad.placement_id, ad.creative_instance_id,
            mojom::NewTabPageAdEventType::kViewed, /*should_fire_event=*/false);
}

TEST_F(BraveAdsNewTabPageAdEventHandlerIfUserHasNotJoinedBraveRewardsTest,
       FireClickedEvent) {
  // Arrange
  const NewTabPageAdInfo ad = BuildAndSaveAd();

  EXPECT_CALL(delegate_mock_, OnDidFireNewTabPageAdServedEvent(ad));
  EXPECT_CALL(delegate_mock_, OnDidFireNewTabPageAdViewedEvent(ad));

  FireEvents(ad.placement_id, ad.creative_instance_id,
             {mojom::NewTabPageAdEventType::kServed,
              mojom::NewTabPageAdEventType::kViewed},
             /*should_fire_event=*/true);

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnDidFireNewTabPageAdClickedEvent(ad));

  FireEvent(ad.placement_id, ad.creative_instance_id,
            mojom::NewTabPageAdEventType::kClicked, /*should_fire_event=*/true);
}

TEST_F(BraveAdsNewTabPageAdEventHandlerIfUserHasNotJoinedBraveRewardsTest,
       DoNotFireClickedEventIfAdPlacementWasAlreadyClicked) {
  // Arrange
  const NewTabPageAdInfo ad = BuildAndSaveAd();

  EXPECT_CALL(delegate_mock_, OnDidFireNewTabPageAdServedEvent(ad));
  EXPECT_CALL(delegate_mock_, OnDidFireNewTabPageAdViewedEvent(ad));
  EXPECT_CALL(delegate_mock_, OnDidFireNewTabPageAdClickedEvent(ad));

  FireEvents(ad.placement_id, ad.creative_instance_id,
             {mojom::NewTabPageAdEventType::kServed,
              mojom::NewTabPageAdEventType::kViewed,
              mojom::NewTabPageAdEventType::kClicked},
             /*should_fire_event=*/true);

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnFailedToFireNewTabPageAdEvent(
                                  ad.placement_id, ad.creative_instance_id,
                                  mojom::NewTabPageAdEventType::kClicked));

  FireEvent(ad.placement_id, ad.creative_instance_id,
            mojom::NewTabPageAdEventType::kClicked,
            /*should_fire_event=*/false);
}

TEST_F(BraveAdsNewTabPageAdEventHandlerIfUserHasNotJoinedBraveRewardsTest,
       DoNotFireClickedEventIfAdPlacementWasNotServed) {
  // Arrange
  const NewTabPageAdInfo ad = BuildAndSaveAd();

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnFailedToFireNewTabPageAdEvent(
                                  ad.placement_id, ad.creative_instance_id,
                                  mojom::NewTabPageAdEventType::kClicked));

  FireEvent(ad.placement_id, ad.creative_instance_id,
            mojom::NewTabPageAdEventType::kClicked,
            /*should_fire_event=*/false);
}

TEST_F(BraveAdsNewTabPageAdEventHandlerIfUserHasNotJoinedBraveRewardsTest,
       DoNotFireEventWithInvalidPlacementId) {
  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnFailedToFireNewTabPageAdEvent(
                                  kInvalidPlacementId, kCreativeInstanceId,
                                  mojom::NewTabPageAdEventType::kServed));

  FireEvent(kInvalidPlacementId, kCreativeInstanceId,
            mojom::NewTabPageAdEventType::kServed,
            /*should_fire_event=*/false);
}

TEST_F(BraveAdsNewTabPageAdEventHandlerIfUserHasNotJoinedBraveRewardsTest,
       DoNotFireEventWithInvalidCreativeInstanceId) {
  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnFailedToFireNewTabPageAdEvent(
                                  kPlacementId, kInvalidCreativeInstanceId,
                                  mojom::NewTabPageAdEventType::kServed));

  FireEvent(kPlacementId, kInvalidCreativeInstanceId,
            mojom::NewTabPageAdEventType::kServed,
            /*should_fire_event=*/false);
}

TEST_F(BraveAdsNewTabPageAdEventHandlerIfUserHasNotJoinedBraveRewardsTest,
       DoNotFireEventForMissingCreativeInstanceId) {
  // Arrange
  const NewTabPageAdInfo ad = BuildAndSaveAd();

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnFailedToFireNewTabPageAdEvent(
                                  ad.placement_id, kMissingCreativeInstanceId,
                                  mojom::NewTabPageAdEventType::kServed));

  FireEvent(ad.placement_id, kMissingCreativeInstanceId,
            mojom::NewTabPageAdEventType::kServed,
            /*should_fire_event=*/false);
}

}  // namespace brave_ads
