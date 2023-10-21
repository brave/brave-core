/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user/user_interaction/ad_events/promoted_content_ads/promoted_content_ad_event_handler.h"

#include <vector>

#include "base/test/mock_callback.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/creatives/promoted_content_ads/creative_promoted_content_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/promoted_content_ads/creative_promoted_content_ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/creatives/promoted_content_ads/creative_promoted_content_ads_database_util.h"
#include "brave/components/brave_ads/core/internal/creatives/promoted_content_ads/promoted_content_ad_builder.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/permission_rules_unittest_util.h"
#include "brave/components/brave_ads/core/internal/units/ad_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/units/promoted_content_ad/promoted_content_ad_feature.h"
#include "brave/components/brave_ads/core/internal/units/promoted_content_ad/promoted_content_ad_info.h"
#include "brave/components/brave_ads/core/internal/user/user_interaction/ad_events/ad_event_builder.h"
#include "brave/components/brave_ads/core/internal/user/user_interaction/ad_events/ad_event_unittest_util.h"
#include "brave/components/brave_ads/core/internal/user/user_interaction/ad_events/promoted_content_ads/promoted_content_ad_event_handler_delegate.h"
#include "brave/components/brave_ads/core/internal/user/user_interaction/ad_events/promoted_content_ads/promoted_content_ad_event_handler_delegate_mock.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

PromotedContentAdInfo BuildAndSaveAd() {
  CreativePromotedContentAdInfo creative_ad =
      test::BuildCreativePromotedContentAd(
          /*should_use_random_uuids=*/false);
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

  void FireEvent(const std::string& placement_id,
                 const std::string& creative_instance_id,
                 const mojom::PromotedContentAdEventType& event_type,
                 const bool should_fire_event) {
    base::MockCallback<FirePromotedContentAdEventHandlerCallback> callback;
    EXPECT_CALL(callback,
                Run(/*success=*/should_fire_event, placement_id, event_type));
    event_handler_.FireEvent(placement_id, creative_instance_id, event_type,
                             callback.Get());
  }

  void FireEvents(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      const std::vector<mojom::PromotedContentAdEventType>& event_types,
      const bool should_fire_event) {
    for (const auto& event_type : event_types) {
      FireEvent(placement_id, creative_instance_id, event_type,
                should_fire_event);
    }
  }

  PromotedContentAdEventHandler event_handler_;
  ::testing::StrictMock<PromotedContentAdEventHandlerDelegateMock>
      delegate_mock_;
};

TEST_F(BraveAdsPromotedContentAdEventHandlerTest, FireViewedEvent) {
  // Arrange
  const PromotedContentAdInfo ad = BuildAndSaveAd();

  EXPECT_CALL(delegate_mock_, OnDidFirePromotedContentAdServedEvent(ad));

  FireEvent(ad.placement_id, ad.creative_instance_id,
            mojom::PromotedContentAdEventType::kServed,
            /*should_fire_event=*/true);

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnDidFirePromotedContentAdViewedEvent(ad));

  FireEvent(ad.placement_id, ad.creative_instance_id,
            mojom::PromotedContentAdEventType::kViewed,
            /*should_fire_event=*/true);
}

TEST_F(BraveAdsPromotedContentAdEventHandlerTest,
       DoNotFireViewedEventIfAdPlacementWasAlreadyViewed) {
  // Arrange
  const PromotedContentAdInfo ad = BuildAndSaveAd();

  EXPECT_CALL(delegate_mock_, OnDidFirePromotedContentAdServedEvent(ad));
  EXPECT_CALL(delegate_mock_, OnDidFirePromotedContentAdViewedEvent(ad));

  FireEvents(ad.placement_id, ad.creative_instance_id,
             {mojom::PromotedContentAdEventType::kServed,
              mojom::PromotedContentAdEventType::kViewed},
             /*should_fire_event=*/true);

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnFailedToFirePromotedContentAdEvent(
                                  ad.placement_id, ad.creative_instance_id,
                                  mojom::PromotedContentAdEventType::kViewed));

  FireEvent(ad.placement_id, ad.creative_instance_id,
            mojom::PromotedContentAdEventType::kViewed,
            /*should_fire_event=*/false);
}

TEST_F(BraveAdsPromotedContentAdEventHandlerTest,
       DoNotFireViewedEventIfAdPlacementWasNotServed) {
  // Arrange
  const PromotedContentAdInfo ad = BuildAndSaveAd();

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnFailedToFirePromotedContentAdEvent(
                                  ad.placement_id, ad.creative_instance_id,
                                  mojom::PromotedContentAdEventType::kViewed));

  FireEvent(ad.placement_id, ad.creative_instance_id,
            mojom::PromotedContentAdEventType::kViewed,
            /*should_fire_event=*/false);
}

TEST_F(BraveAdsPromotedContentAdEventHandlerTest, FireClickedEvent) {
  // Arrange
  const PromotedContentAdInfo ad = BuildAndSaveAd();

  EXPECT_CALL(delegate_mock_, OnDidFirePromotedContentAdServedEvent(ad));
  EXPECT_CALL(delegate_mock_, OnDidFirePromotedContentAdViewedEvent(ad));

  FireEvents(ad.placement_id, ad.creative_instance_id,
             {mojom::PromotedContentAdEventType::kServed,
              mojom::PromotedContentAdEventType::kViewed},
             /*should_fire_event=*/true);

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnDidFirePromotedContentAdClickedEvent(ad));

  FireEvent(ad.placement_id, ad.creative_instance_id,
            mojom::PromotedContentAdEventType::kClicked,
            /*should_fire_event=*/true);
}

TEST_F(BraveAdsPromotedContentAdEventHandlerTest,
       DoNotFireClickedEventIfAdPlacementWasAlreadyClicked) {
  // Arrange
  const PromotedContentAdInfo ad = BuildAndSaveAd();

  EXPECT_CALL(delegate_mock_, OnDidFirePromotedContentAdServedEvent(ad));
  EXPECT_CALL(delegate_mock_, OnDidFirePromotedContentAdViewedEvent(ad));
  EXPECT_CALL(delegate_mock_, OnDidFirePromotedContentAdClickedEvent(ad));

  FireEvents(ad.placement_id, ad.creative_instance_id,
             {mojom::PromotedContentAdEventType::kServed,
              mojom::PromotedContentAdEventType::kViewed,
              mojom::PromotedContentAdEventType::kClicked},
             /*should_fire_event=*/true);

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnFailedToFirePromotedContentAdEvent(
                                  ad.placement_id, ad.creative_instance_id,
                                  mojom::PromotedContentAdEventType::kClicked));

  FireEvent(ad.placement_id, ad.creative_instance_id,
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

  FireEvent(ad.placement_id, ad.creative_instance_id,
            mojom::PromotedContentAdEventType::kClicked,
            /*should_fire_event=*/false);
}

TEST_F(BraveAdsPromotedContentAdEventHandlerTest,
       DoNotFireEventWithInvalidPlacementId) {
  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnFailedToFirePromotedContentAdEvent(
                                  kInvalidPlacementId, kCreativeInstanceId,
                                  mojom::PromotedContentAdEventType::kServed));

  FireEvent(kInvalidPlacementId, kCreativeInstanceId,
            mojom::PromotedContentAdEventType::kServed,
            /*should_fire_event=*/false);
}

TEST_F(BraveAdsPromotedContentAdEventHandlerTest,
       DoNotFireEventWithInvalidCreativeInstanceId) {
  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnFailedToFirePromotedContentAdEvent(
                                  kPlacementId, kInvalidCreativeInstanceId,
                                  mojom::PromotedContentAdEventType::kServed));

  FireEvent(kPlacementId, kInvalidCreativeInstanceId,
            mojom::PromotedContentAdEventType::kServed,
            /*should_fire_event=*/false);
}

TEST_F(BraveAdsPromotedContentAdEventHandlerTest,
       DoNotFireEventForMissingCreativeInstanceId) {
  // Arrange
  const PromotedContentAdInfo ad = BuildAndSaveAd();

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnFailedToFirePromotedContentAdEvent(
                                  ad.placement_id, kMissingCreativeInstanceId,
                                  mojom::PromotedContentAdEventType::kServed));

  FireEvent(ad.placement_id, kMissingCreativeInstanceId,
            mojom::PromotedContentAdEventType::kServed,
            /*should_fire_event=*/false);
}

TEST_F(BraveAdsPromotedContentAdEventHandlerTest,
       FireEventIfNotExceededAdsPerHourCap) {
  // Arrange
  const PromotedContentAdInfo ad = BuildAndSaveAd();

  const AdEventInfo ad_event =
      BuildAdEvent(ad, ConfirmationType::kServed, /*created_at=*/Now());
  test::RecordAdEvents(ad_event, kMaximumPromotedContentAdsPerHour.Get() - 1);

  AdvanceClockBy(base::Hours(1) - base::Milliseconds(1));

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnDidFirePromotedContentAdServedEvent(ad));

  FireEvent(ad.placement_id, ad.creative_instance_id,
            mojom::PromotedContentAdEventType::kServed,
            /*should_fire_event=*/true);
}

TEST_F(BraveAdsPromotedContentAdEventHandlerTest,
       DoNotFireEventIfExceededAdsPerHourCap) {
  // Arrange
  const PromotedContentAdInfo ad = BuildAndSaveAd();

  const AdEventInfo ad_event =
      BuildAdEvent(ad, ConfirmationType::kServed, /*created_at=*/Now());
  test::RecordAdEvents(ad_event, kMaximumPromotedContentAdsPerHour.Get());

  AdvanceClockBy(base::Hours(1) - base::Milliseconds(1));

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnFailedToFirePromotedContentAdEvent(
                                  ad.placement_id, ad.creative_instance_id,
                                  mojom::PromotedContentAdEventType::kServed));

  FireEvent(ad.placement_id, ad.creative_instance_id,
            mojom::PromotedContentAdEventType::kServed,
            /*should_fire_event=*/false);
}

TEST_F(BraveAdsPromotedContentAdEventHandlerTest,
       FireEventIfNotExceededAdsPerDayCap) {
  // Arrange
  const PromotedContentAdInfo ad = BuildAndSaveAd();

  const AdEventInfo ad_event =
      BuildAdEvent(ad, ConfirmationType::kServed, /*created_at=*/Now());
  test::RecordAdEvents(ad_event, kMaximumPromotedContentAdsPerDay.Get() - 1);

  AdvanceClockBy(base::Days(1) - base::Milliseconds(1));

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnDidFirePromotedContentAdServedEvent(ad));

  FireEvent(ad.placement_id, ad.creative_instance_id,
            mojom::PromotedContentAdEventType::kServed,
            /*should_fire_event=*/true);
}

TEST_F(BraveAdsPromotedContentAdEventHandlerTest,
       DoNotFireEventIfExceededAdsPerDayCap) {
  // Arrange
  const PromotedContentAdInfo ad = BuildAndSaveAd();

  const AdEventInfo ad_event =
      BuildAdEvent(ad, ConfirmationType::kServed, /*created_at=*/Now());
  test::RecordAdEvents(ad_event, kMaximumPromotedContentAdsPerDay.Get());

  AdvanceClockBy(base::Days(1) - base::Milliseconds(1));

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnFailedToFirePromotedContentAdEvent(
                                  ad.placement_id, ad.creative_instance_id,
                                  mojom::PromotedContentAdEventType::kServed));

  FireEvent(ad.placement_id, ad.creative_instance_id,
            mojom::PromotedContentAdEventType::kServed,
            /*should_fire_event=*/false);
}

}  // namespace brave_ads
