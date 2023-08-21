/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/ad_events/promoted_content_ads/promoted_content_ad_event_handler.h"

#include <vector>

#include "base/test/mock_callback.h"
#include "brave/components/brave_ads/core/ad_type.h"
#include "brave/components/brave_ads/core/confirmation_type.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/ad_event_builder.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/ad_event_unittest_util.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/promoted_content_ads/promoted_content_ad_event_handler_delegate.h"
#include "brave/components/brave_ads/core/internal/ads/ad_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/ads/ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/ads/promoted_content_ad_feature.h"
#include "brave/components/brave_ads/core/internal/ads/serving/permission_rules/permission_rules_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/creatives/promoted_content_ads/creative_promoted_content_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/promoted_content_ads/creative_promoted_content_ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/creatives/promoted_content_ads/creative_promoted_content_ads_database_util.h"
#include "brave/components/brave_ads/core/internal/creatives/promoted_content_ads/promoted_content_ad_builder.h"
#include "brave/components/brave_ads/core/promoted_content_ad_info.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

CreativePromotedContentAdInfo BuildAndSaveCreativeAd() {
  CreativePromotedContentAdInfo creative_ad =
      BuildCreativePromotedContentAdForTesting(
          /*should_use_random_uuids*/ false);
  database::SaveCreativePromotedContentAds({creative_ad});
  return creative_ad;
}

}  // namespace

class BraveAdsPromotedContentAdEventHandlerTest
    : public PromotedContentAdEventHandlerDelegate,
      public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    event_handler_.SetDelegate(this);

    ForcePermissionRulesForTesting();
  }

  void OnDidFirePromotedContentAdServedEvent(
      const PromotedContentAdInfo& ad) override {
    ad_ = ad;
    did_serve_ad_ = true;
  }

  void OnDidFirePromotedContentAdViewedEvent(
      const PromotedContentAdInfo& ad) override {
    ad_ = ad;
    did_view_ad_ = true;
  }

  void OnDidFirePromotedContentAdClickedEvent(
      const PromotedContentAdInfo& ad) override {
    ad_ = ad;
    did_click_ad_ = true;
  }

  void OnFailedToFirePromotedContentAdEvent(
      const std::string& /*placement_id*/,
      const std::string& /*creative_instance_id*/,
      const mojom::PromotedContentAdEventType /*event_type*/) override {
    did_fail_to_fire_event_ = true;
  }

  void FireEvent(const std::string& placement_id,
                 const std::string& creative_instance_id,
                 const mojom::PromotedContentAdEventType& event_type,
                 const bool should_fire_event) {
    base::MockCallback<FirePromotedContentAdEventHandlerCallback> callback;
    EXPECT_CALL(callback,
                Run(/*success*/ should_fire_event, placement_id, event_type));

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

  PromotedContentAdInfo ad_;
  bool did_serve_ad_ = false;
  bool did_view_ad_ = false;
  bool did_click_ad_ = false;
  bool did_fail_to_fire_event_ = false;
};

TEST_F(BraveAdsPromotedContentAdEventHandlerTest, FireViewedEvent) {
  // Arrange
  const CreativePromotedContentAdInfo creative_ad = BuildAndSaveCreativeAd();

  FireEvent(kPlacementId, creative_ad.creative_instance_id,
            mojom::PromotedContentAdEventType::kServed,
            /*should_fire_event*/ true);

  // Act
  FireEvent(kPlacementId, creative_ad.creative_instance_id,
            mojom::PromotedContentAdEventType::kViewed,
            /*should_fire_event*/ true);

  // Assert
  EXPECT_TRUE(did_serve_ad_);
  EXPECT_TRUE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_FALSE(did_fail_to_fire_event_);
  EXPECT_EQ(BuildPromotedContentAd(creative_ad, kPlacementId), ad_);
  EXPECT_EQ(1U, GetAdEventCountForTesting(AdType::kPromotedContentAd,
                                          ConfirmationType::kServed));
  EXPECT_EQ(1U, GetAdEventCountForTesting(AdType::kPromotedContentAd,
                                          ConfirmationType::kViewed));
}

TEST_F(BraveAdsPromotedContentAdEventHandlerTest,
       DoNotFireViewedEventIfAdPlacementWasAlreadyViewed) {
  // Arrange
  const CreativePromotedContentAdInfo creative_ad = BuildAndSaveCreativeAd();

  FireEvents(kPlacementId, creative_ad.creative_instance_id,
             {mojom::PromotedContentAdEventType::kServed,
              mojom::PromotedContentAdEventType::kViewed},
             /*should_fire_event*/ true);

  // Act
  FireEvent(kPlacementId, creative_ad.creative_instance_id,
            mojom::PromotedContentAdEventType::kViewed,
            /*should_fire_event*/ false);

  // Assert
  EXPECT_EQ(1U, GetAdEventCountForTesting(AdType::kPromotedContentAd,
                                          ConfirmationType::kServed));
  EXPECT_EQ(1U, GetAdEventCountForTesting(AdType::kPromotedContentAd,
                                          ConfirmationType::kViewed));
}

TEST_F(BraveAdsPromotedContentAdEventHandlerTest,
       DoNotFireViewedEventIfAdPlacementWasNotServed) {
  // Arrange
  const CreativePromotedContentAdInfo creative_ad = BuildAndSaveCreativeAd();

  // Act
  FireEvent(kPlacementId, creative_ad.creative_instance_id,
            mojom::PromotedContentAdEventType::kViewed,
            /*should_fire_event*/ false);

  // Assert
  EXPECT_EQ(0U, GetAdEventCountForTesting(AdType::kPromotedContentAd,
                                          ConfirmationType::kServed));
  EXPECT_EQ(0U, GetAdEventCountForTesting(AdType::kPromotedContentAd,
                                          ConfirmationType::kViewed));
}

TEST_F(BraveAdsPromotedContentAdEventHandlerTest, FireClickedEvent) {
  // Arrange
  const CreativePromotedContentAdInfo creative_ad = BuildAndSaveCreativeAd();

  FireEvents(kPlacementId, creative_ad.creative_instance_id,
             {mojom::PromotedContentAdEventType::kServed,
              mojom::PromotedContentAdEventType::kViewed},
             /*should_fire_event*/ true);

  // Act
  FireEvent(kPlacementId, creative_ad.creative_instance_id,
            mojom::PromotedContentAdEventType::kClicked,
            /*should_fire_event*/ true);

  // Assert
  EXPECT_TRUE(did_serve_ad_);
  EXPECT_TRUE(did_view_ad_);
  EXPECT_TRUE(did_click_ad_);
  EXPECT_FALSE(did_fail_to_fire_event_);
  EXPECT_EQ(BuildPromotedContentAd(creative_ad, kPlacementId), ad_);
  EXPECT_EQ(1U, GetAdEventCountForTesting(AdType::kPromotedContentAd,
                                          ConfirmationType::kServed));
  EXPECT_EQ(1U, GetAdEventCountForTesting(AdType::kPromotedContentAd,
                                          ConfirmationType::kViewed));
  EXPECT_EQ(1U, GetAdEventCountForTesting(AdType::kPromotedContentAd,
                                          ConfirmationType::kClicked));
}

TEST_F(BraveAdsPromotedContentAdEventHandlerTest,
       DoNotFireClickedEventIfAdPlacementWasAlreadyClicked) {
  // Arrange
  const CreativePromotedContentAdInfo creative_ad = BuildAndSaveCreativeAd();

  FireEvents(kPlacementId, creative_ad.creative_instance_id,
             {mojom::PromotedContentAdEventType::kServed,
              mojom::PromotedContentAdEventType::kViewed,
              mojom::PromotedContentAdEventType::kClicked},
             /*should_fire_event*/ true);

  // Act
  FireEvent(kPlacementId, creative_ad.creative_instance_id,
            mojom::PromotedContentAdEventType::kClicked,
            /*should_fire_event*/ false);

  // Assert
  EXPECT_EQ(1U, GetAdEventCountForTesting(AdType::kPromotedContentAd,
                                          ConfirmationType::kServed));
  EXPECT_EQ(1U, GetAdEventCountForTesting(AdType::kPromotedContentAd,
                                          ConfirmationType::kViewed));
  EXPECT_EQ(1U, GetAdEventCountForTesting(AdType::kPromotedContentAd,
                                          ConfirmationType::kClicked));
}

TEST_F(BraveAdsPromotedContentAdEventHandlerTest,
       DoNotFireClickedEventIfAdPlacementWasNotServed) {
  // Arrange
  const CreativePromotedContentAdInfo creative_ad = BuildAndSaveCreativeAd();

  // Act
  FireEvent(kPlacementId, creative_ad.creative_instance_id,
            mojom::PromotedContentAdEventType::kViewed,
            /*should_fire_event*/ false);

  // Assert
  EXPECT_EQ(0U, GetAdEventCountForTesting(AdType::kPromotedContentAd,
                                          ConfirmationType::kServed));
  EXPECT_EQ(0U, GetAdEventCountForTesting(AdType::kPromotedContentAd,
                                          ConfirmationType::kViewed));
}

TEST_F(BraveAdsPromotedContentAdEventHandlerTest,
       DoNotFireEventWithInvalidPlacementId) {
  // Arrange

  // Act
  FireEvent(kInvalidPlacementId, kCreativeInstanceId,
            mojom::PromotedContentAdEventType::kServed,
            /*should_fire_event*/ false);

  // Assert
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_FALSE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_TRUE(did_fail_to_fire_event_);
  EXPECT_EQ(0U, GetAdEventCountForTesting(AdType::kPromotedContentAd,
                                          ConfirmationType::kServed));
}

TEST_F(BraveAdsPromotedContentAdEventHandlerTest,
       DoNotFireEventWithInvalidCreativeInstanceId) {
  // Arrange

  // Act
  FireEvent(kPlacementId, kInvalidCreativeInstanceId,
            mojom::PromotedContentAdEventType::kServed,
            /*should_fire_event*/ false);

  // Assert
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_FALSE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_TRUE(did_fail_to_fire_event_);
  EXPECT_EQ(0U, GetAdEventCountForTesting(AdType::kPromotedContentAd,
                                          ConfirmationType::kViewed));
}

TEST_F(BraveAdsPromotedContentAdEventHandlerTest,
       DoNotFireEventForMissingCreativeInstanceId) {
  // Arrange
  const CreativePromotedContentAdInfo creative_ad = BuildAndSaveCreativeAd();

  // Act
  FireEvent(kPlacementId, kMissingCreativeInstanceId,
            mojom::PromotedContentAdEventType::kServed,
            /*should_fire_event*/ false);

  // Assert
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_FALSE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_TRUE(did_fail_to_fire_event_);
  EXPECT_EQ(0U, GetAdEventCountForTesting(AdType::kPromotedContentAd,
                                          ConfirmationType::kServed));
}

TEST_F(BraveAdsPromotedContentAdEventHandlerTest,
       FireEventIfNotExceededAdsPerHourCap) {
  // Arrange
  ForcePermissionRulesForTesting();

  BuildAndSaveCreativeAd();

  const AdInfo ad = BuildAdForTesting(AdType::kPromotedContentAd,
                                      /*should_use_random_uuids*/ false);
  const AdEventInfo ad_event =
      BuildAdEvent(ad, ConfirmationType::kServed, /*created_at*/ Now());

  const size_t ads_per_hour = kMaximumPromotedContentAdsPerHour.Get();

  FireAdEventsForTesting(ad_event, ads_per_hour - 1);

  AdvanceClockBy(base::Hours(1) - base::Milliseconds(1));

  // Act
  FireEvent(ad.placement_id, ad.creative_instance_id,
            mojom::PromotedContentAdEventType::kServed,
            /*should_fire_event*/ true);

  // Assert
  EXPECT_EQ(ads_per_hour, GetAdEventCountForTesting(AdType::kPromotedContentAd,
                                                    ConfirmationType::kServed));
}

TEST_F(BraveAdsPromotedContentAdEventHandlerTest,
       DoNotFireEventIfExceededAdsPerHourCap) {
  // Arrange
  ForcePermissionRulesForTesting();

  BuildAndSaveCreativeAd();

  const AdInfo ad = BuildAdForTesting(AdType::kPromotedContentAd,
                                      /*should_use_random_uuids*/ false);
  const AdEventInfo ad_event =
      BuildAdEvent(ad, ConfirmationType::kServed, /*created_at*/ Now());

  const size_t ads_per_hour = kMaximumPromotedContentAdsPerHour.Get();

  FireAdEventsForTesting(ad_event, ads_per_hour);

  AdvanceClockBy(base::Hours(1) - base::Milliseconds(1));

  // Act
  FireEvent(ad.placement_id, ad.creative_instance_id,
            mojom::PromotedContentAdEventType::kServed,
            /*should_fire_event*/ false);

  // Assert
  EXPECT_EQ(ads_per_hour, GetAdEventCountForTesting(AdType::kPromotedContentAd,
                                                    ConfirmationType::kServed));
}

TEST_F(BraveAdsPromotedContentAdEventHandlerTest,
       FireEventIfNotExceededAdsPerDayCap) {
  // Arrange
  ForcePermissionRulesForTesting();

  BuildAndSaveCreativeAd();

  const AdInfo ad = BuildAdForTesting(AdType::kPromotedContentAd,
                                      /*should_use_random_uuids*/ false);
  const AdEventInfo ad_event =
      BuildAdEvent(ad, ConfirmationType::kServed, /*created_at*/ Now());

  const size_t ads_per_day = kMaximumPromotedContentAdsPerDay.Get();

  FireAdEventsForTesting(ad_event, ads_per_day - 1);

  AdvanceClockBy(base::Days(1) - base::Milliseconds(1));

  // Act
  FireEvent(ad.placement_id, ad.creative_instance_id,
            mojom::PromotedContentAdEventType::kServed,
            /*should_fire_event*/ true);

  // Assert
  EXPECT_EQ(ads_per_day, GetAdEventCountForTesting(AdType::kPromotedContentAd,
                                                   ConfirmationType::kServed));
}

TEST_F(BraveAdsPromotedContentAdEventHandlerTest,
       DoNotFireEventIfExceededAdsPerDayCap) {
  // Arrange
  ForcePermissionRulesForTesting();

  BuildAndSaveCreativeAd();

  const AdInfo ad = BuildAdForTesting(AdType::kPromotedContentAd,
                                      /*should_use_random_uuids*/ false);
  const AdEventInfo ad_event =
      BuildAdEvent(ad, ConfirmationType::kServed, /*created_at*/ Now());

  const size_t ads_per_day = kMaximumPromotedContentAdsPerDay.Get();

  FireAdEventsForTesting(ad_event, ads_per_day);

  AdvanceClockBy(base::Days(1) - base::Milliseconds(1));

  // Act
  FireEvent(ad.placement_id, ad.creative_instance_id,
            mojom::PromotedContentAdEventType::kServed,
            /*should_fire_event*/ false);

  // Assert
  EXPECT_EQ(ads_per_day, GetAdEventCountForTesting(AdType::kPromotedContentAd,
                                                   ConfirmationType::kServed));
}

}  // namespace brave_ads
