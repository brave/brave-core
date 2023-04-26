/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/ad_events/promoted_content_ads/promoted_content_ad_event_handler.h"

#include "base/guid.h"
#include "brave/components/brave_ads/core/ad_type.h"
#include "brave/components/brave_ads/core/confirmation_type.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/ad_event_info.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/ad_event_unittest_util.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/promoted_content_ads/promoted_content_ad_event_handler_delegate.h"
#include "brave/components/brave_ads/core/internal/ads/ad_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/ads/promoted_content_ad_feature.h"
#include "brave/components/brave_ads/core/internal/ads/serving/permission_rules/permission_rules_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/creatives/promoted_content_ads/creative_promoted_content_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/promoted_content_ads/creative_promoted_content_ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/creatives/promoted_content_ads/promoted_content_ad_builder.h"
#include "brave/components/brave_ads/core/promoted_content_ad_info.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

CreativePromotedContentAdInfo BuildAndSaveCreativeAd() {
  CreativePromotedContentAdInfo creative_ad =
      BuildCreativePromotedContentAd(/*should_use_random_guids*/ true);

  SaveCreativeAds({creative_ad});

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
  }

  void OnPromotedContentAdServed(const PromotedContentAdInfo& ad) override {
    ad_ = ad;
    did_serve_ad_ = true;
  }

  void OnPromotedContentAdViewed(const PromotedContentAdInfo& ad) override {
    ad_ = ad;
    did_view_ad_ = true;
  }

  void OnPromotedContentAdClicked(const PromotedContentAdInfo& ad) override {
    ad_ = ad;
    did_click_ad_ = true;
  }

  void OnPromotedContentAdEventFailed(
      const std::string& /*placement_id*/,
      const std::string& /*creative_instance_id*/,
      const mojom::PromotedContentAdEventType /*event_type*/) override {
    did_fail_to_fire_event_ = true;
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
  ForcePermissionRulesForTesting();

  const CreativePromotedContentAdInfo creative_ad = BuildAndSaveCreativeAd();

  event_handler_.FireEvent(kPlacementId, creative_ad.creative_instance_id,
                           mojom::PromotedContentAdEventType::kServed);

  // Act
  event_handler_.FireEvent(kPlacementId, creative_ad.creative_instance_id,
                           mojom::PromotedContentAdEventType::kViewed);

  // Assert
  EXPECT_TRUE(did_serve_ad_);
  EXPECT_TRUE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_FALSE(did_fail_to_fire_event_);
  const PromotedContentAdInfo expected_ad =
      BuildPromotedContentAd(creative_ad, kPlacementId);
  EXPECT_EQ(expected_ad, ad_);
  EXPECT_EQ(1U, GetAdEventCount(AdType::kPromotedContentAd,
                                ConfirmationType::kServed));
  EXPECT_EQ(1U, GetAdEventCount(AdType::kPromotedContentAd,
                                ConfirmationType::kViewed));
}

TEST_F(BraveAdsPromotedContentAdEventHandlerTest,
       DoNotFireViewedEventIfAlreadyFired) {
  // Arrange
  ForcePermissionRulesForTesting();

  const CreativePromotedContentAdInfo creative_ad = BuildAndSaveCreativeAd();

  event_handler_.FireEvent(kPlacementId, creative_ad.creative_instance_id,
                           mojom::PromotedContentAdEventType::kServed);

  event_handler_.FireEvent(kPlacementId, creative_ad.creative_instance_id,
                           mojom::PromotedContentAdEventType::kViewed);

  // Act
  event_handler_.FireEvent(kPlacementId, creative_ad.creative_instance_id,
                           mojom::PromotedContentAdEventType::kViewed);

  // Assert
  EXPECT_EQ(1U, GetAdEventCount(AdType::kPromotedContentAd,
                                ConfirmationType::kServed));
  EXPECT_EQ(1U, GetAdEventCount(AdType::kPromotedContentAd,
                                ConfirmationType::kViewed));
}

TEST_F(BraveAdsPromotedContentAdEventHandlerTest, FireClickedEvent) {
  // Arrange
  ForcePermissionRulesForTesting();

  const CreativePromotedContentAdInfo creative_ad = BuildAndSaveCreativeAd();

  event_handler_.FireEvent(kPlacementId, creative_ad.creative_instance_id,
                           mojom::PromotedContentAdEventType::kServed);

  event_handler_.FireEvent(kPlacementId, creative_ad.creative_instance_id,
                           mojom::PromotedContentAdEventType::kViewed);

  // Act
  event_handler_.FireEvent(kPlacementId, creative_ad.creative_instance_id,
                           mojom::PromotedContentAdEventType::kClicked);

  // Assert
  EXPECT_TRUE(did_serve_ad_);
  EXPECT_TRUE(did_view_ad_);
  EXPECT_TRUE(did_click_ad_);
  EXPECT_FALSE(did_fail_to_fire_event_);
  const PromotedContentAdInfo expected_ad =
      BuildPromotedContentAd(creative_ad, kPlacementId);
  EXPECT_EQ(expected_ad, ad_);
  EXPECT_EQ(1U, GetAdEventCount(AdType::kPromotedContentAd,
                                ConfirmationType::kServed));
  EXPECT_EQ(1U, GetAdEventCount(AdType::kPromotedContentAd,
                                ConfirmationType::kViewed));
  EXPECT_EQ(1U, GetAdEventCount(AdType::kPromotedContentAd,
                                ConfirmationType::kClicked));
}

TEST_F(BraveAdsPromotedContentAdEventHandlerTest,
       DoNotFireClickedEventIfMissingAdPlacement) {
  // Arrange
  const CreativePromotedContentAdInfo creative_ad = BuildAndSaveCreativeAd();

  // Act
  event_handler_.FireEvent(kPlacementId, creative_ad.creative_instance_id,
                           mojom::PromotedContentAdEventType::kClicked);

  // Assert
  EXPECT_EQ(0U, GetAdEventCount(AdType::kPromotedContentAd,
                                ConfirmationType::kClicked));
}

TEST_F(BraveAdsPromotedContentAdEventHandlerTest,
       DoNotFireClickedEventIfAlreadyFired) {
  // Arrange
  ForcePermissionRulesForTesting();

  const CreativePromotedContentAdInfo creative_ad = BuildAndSaveCreativeAd();

  event_handler_.FireEvent(kPlacementId, creative_ad.creative_instance_id,
                           mojom::PromotedContentAdEventType::kServed);
  event_handler_.FireEvent(kPlacementId, creative_ad.creative_instance_id,
                           mojom::PromotedContentAdEventType::kViewed);
  event_handler_.FireEvent(kPlacementId, creative_ad.creative_instance_id,
                           mojom::PromotedContentAdEventType::kClicked);

  // Act
  event_handler_.FireEvent(kPlacementId, creative_ad.creative_instance_id,
                           mojom::PromotedContentAdEventType::kClicked);

  // Assert
  EXPECT_EQ(1U, GetAdEventCount(AdType::kPromotedContentAd,
                                ConfirmationType::kServed));
  EXPECT_EQ(1U, GetAdEventCount(AdType::kPromotedContentAd,
                                ConfirmationType::kViewed));
  EXPECT_EQ(1U, GetAdEventCount(AdType::kPromotedContentAd,
                                ConfirmationType::kClicked));
}

TEST_F(BraveAdsPromotedContentAdEventHandlerTest,
       DoNotFireEventWithInvalidPlacementId) {
  // Arrange

  // Act
  event_handler_.FireEvent(kInvalidPlacementId, kCreativeInstanceId,
                           mojom::PromotedContentAdEventType::kServed);

  // Assert
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_FALSE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_TRUE(did_fail_to_fire_event_);
  EXPECT_EQ(0U, GetAdEventCount(AdType::kPromotedContentAd,
                                ConfirmationType::kServed));
}

TEST_F(BraveAdsPromotedContentAdEventHandlerTest,
       DoNotFireEventWithInvalidCreativeInstanceId) {
  // Arrange

  // Act
  event_handler_.FireEvent(kPlacementId, kInvalidCreativeInstanceId,
                           mojom::PromotedContentAdEventType::kViewed);

  // Assert
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_FALSE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_TRUE(did_fail_to_fire_event_);
  EXPECT_EQ(0U, GetAdEventCount(AdType::kPromotedContentAd,
                                ConfirmationType::kViewed));
}

TEST_F(BraveAdsPromotedContentAdEventHandlerTest,
       DoNotFireEventWhenNotPermitted) {
  // Arrange
  const CreativePromotedContentAdInfo creative_ad = BuildAndSaveCreativeAd();

  // Act
  event_handler_.FireEvent(kPlacementId, creative_ad.creative_instance_id,
                           mojom::PromotedContentAdEventType::kViewed);

  // Assert
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_FALSE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_TRUE(did_fail_to_fire_event_);
  EXPECT_EQ(0U, GetAdEventCount(AdType::kPromotedContentAd,
                                ConfirmationType::kViewed));
}

TEST_F(BraveAdsPromotedContentAdEventHandlerTest,
       DoNotFireEventForUnknownCreativeInstanceId) {
  // Arrange
  ForcePermissionRulesForTesting();

  // Act
  event_handler_.FireEvent(kPlacementId, kCreativeInstanceId,
                           mojom::PromotedContentAdEventType::kViewed);

  // Assert
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_FALSE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_TRUE(did_fail_to_fire_event_);
  EXPECT_EQ(0U, GetAdEventCount(AdType::kPromotedContentAd,
                                ConfirmationType::kViewed));
}

TEST_F(BraveAdsPromotedContentAdEventHandlerTest,
       FireEventIfNotExceededAdsPerHourCap) {
  // Arrange
  ForcePermissionRulesForTesting();

  const CreativePromotedContentAdInfo creative_ad = BuildAndSaveCreativeAd();
  const AdEventInfo ad_event =
      BuildAdEvent(creative_ad, AdType::kPromotedContentAd,
                   ConfirmationType::kServed, Now());

  const size_t ads_per_hour = kMaximumPromotedContentAdsPerHour.Get();

  FireAdEvents(ad_event, ads_per_hour - 1);

  const std::string placement_id =
      base::GUID::GenerateRandomV4().AsLowercaseString();

  // Act
  event_handler_.FireEvent(placement_id, creative_ad.creative_instance_id,
                           mojom::PromotedContentAdEventType::kServed);

  // Assert
  EXPECT_EQ(ads_per_hour, GetAdEventCount(AdType::kPromotedContentAd,
                                          ConfirmationType::kServed));
}

TEST_F(BraveAdsPromotedContentAdEventHandlerTest,
       DoNotFireEventIfExceededAdsPerHourCap) {
  // Arrange
  ForcePermissionRulesForTesting();

  const CreativePromotedContentAdInfo creative_ad = BuildAndSaveCreativeAd();
  const AdEventInfo ad_event =
      BuildAdEvent(creative_ad, AdType::kPromotedContentAd,
                   ConfirmationType::kServed, Now());

  const size_t ads_per_hour = kMaximumPromotedContentAdsPerHour.Get();

  FireAdEvents(ad_event, ads_per_hour);

  const std::string placement_id =
      base::GUID::GenerateRandomV4().AsLowercaseString();

  // Act
  event_handler_.FireEvent(placement_id, creative_ad.creative_instance_id,
                           mojom::PromotedContentAdEventType::kServed);

  // Assert
  EXPECT_EQ(ads_per_hour, GetAdEventCount(AdType::kPromotedContentAd,
                                          ConfirmationType::kServed));
}

TEST_F(BraveAdsPromotedContentAdEventHandlerTest,
       FireEventIfNotExceededAdsPerDayCap) {
  // Arrange
  ForcePermissionRulesForTesting();

  const CreativePromotedContentAdInfo creative_ad = BuildAndSaveCreativeAd();
  const AdEventInfo ad_event =
      BuildAdEvent(creative_ad, AdType::kPromotedContentAd,
                   ConfirmationType::kServed, Now());

  const size_t ads_per_day = kMaximumPromotedContentAdsPerDay.Get();

  FireAdEvents(ad_event, ads_per_day - 1);

  AdvanceClockBy(base::Hours(1));

  const std::string placement_id =
      base::GUID::GenerateRandomV4().AsLowercaseString();

  // Act
  event_handler_.FireEvent(placement_id, creative_ad.creative_instance_id,
                           mojom::PromotedContentAdEventType::kServed);

  // Assert
  EXPECT_EQ(ads_per_day, GetAdEventCount(AdType::kPromotedContentAd,
                                         ConfirmationType::kServed));
}

TEST_F(BraveAdsPromotedContentAdEventHandlerTest,
       DoNotFireEventIfExceededAdsPerDayCap) {
  // Arrange
  ForcePermissionRulesForTesting();

  const CreativePromotedContentAdInfo creative_ad = BuildAndSaveCreativeAd();
  const AdEventInfo ad_event =
      BuildAdEvent(creative_ad, AdType::kPromotedContentAd,
                   ConfirmationType::kServed, Now());

  const size_t ads_per_day = kMaximumPromotedContentAdsPerDay.Get();

  FireAdEvents(ad_event, ads_per_day);

  AdvanceClockBy(base::Hours(1));

  const std::string placement_id =
      base::GUID::GenerateRandomV4().AsLowercaseString();

  // Act
  event_handler_.FireEvent(placement_id, creative_ad.creative_instance_id,
                           mojom::PromotedContentAdEventType::kServed);

  // Assert
  EXPECT_EQ(ads_per_day, GetAdEventCount(AdType::kPromotedContentAd,
                                         ConfirmationType::kServed));
}

}  // namespace brave_ads
