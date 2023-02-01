/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/ad_events/promoted_content_ads/promoted_content_ad_event_handler.h"

#include <memory>

#include "base/guid.h"
#include "bat/ads/ad_type.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/ads/ad_events/ad_event_info.h"
#include "bat/ads/internal/ads/ad_events/ad_event_unittest_util.h"
#include "bat/ads/internal/ads/ad_events/promoted_content_ads/promoted_content_ad_event_handler_observer.h"
#include "bat/ads/internal/ads/serving/permission_rules/permission_rules_unittest_util.h"
#include "bat/ads/internal/ads/serving/serving_features.h"
#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/common/unittest/unittest_time_util.h"
#include "bat/ads/internal/creatives/promoted_content_ads/creative_promoted_content_ad_info.h"
#include "bat/ads/internal/creatives/promoted_content_ads/creative_promoted_content_ad_unittest_util.h"
#include "bat/ads/internal/creatives/promoted_content_ads/promoted_content_ad_builder.h"
#include "bat/ads/promoted_content_ad_info.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads::promoted_content_ads {

namespace {

constexpr char kPlacementId[] = "d2ef9bb0-a0dc-472c-bc49-62105bb6da68";
constexpr char kInvalidPlacementId[] = "";

constexpr char kCreativeInstanceId[] = "1547f94f-9086-4db9-a441-efb2f0365269";
constexpr char kInvalidCreativeInstanceId[] = "";

CreativePromotedContentAdInfo BuildAndSaveCreativeAd() {
  CreativePromotedContentAdList creative_ads;
  CreativePromotedContentAdInfo creative_ad = BuildCreativePromotedContentAd();
  creative_ads.push_back(creative_ad);

  SaveCreativeAds(creative_ads);

  return creative_ad;
}

}  // namespace

class BatAdsPromotedContentAdEventHandlerTest : public EventHandlerObserver,
                                                public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    event_handler_ = std::make_unique<EventHandler>();
    event_handler_->AddObserver(this);
  }

  void TearDown() override {
    event_handler_->RemoveObserver(this);

    UnitTestBase::TearDown();
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

  std::unique_ptr<EventHandler> event_handler_;

  PromotedContentAdInfo ad_;
  bool did_serve_ad_ = false;
  bool did_view_ad_ = false;
  bool did_click_ad_ = false;
  bool did_fail_to_fire_event_ = false;
};

TEST_F(BatAdsPromotedContentAdEventHandlerTest, FireViewedEvent) {
  // Arrange
  ForcePermissionRulesForTesting();

  const CreativePromotedContentAdInfo creative_ad = BuildAndSaveCreativeAd();

  event_handler_->FireEvent(kPlacementId, creative_ad.creative_instance_id,
                            mojom::PromotedContentAdEventType::kServed);

  // Act
  event_handler_->FireEvent(kPlacementId, creative_ad.creative_instance_id,
                            mojom::PromotedContentAdEventType::kViewed);

  // Assert
  EXPECT_TRUE(did_serve_ad_);
  EXPECT_TRUE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_FALSE(did_fail_to_fire_event_);
  const PromotedContentAdInfo expected_ad =
      BuildPromotedContentAd(creative_ad, kPlacementId);
  EXPECT_EQ(expected_ad, ad_);
  EXPECT_EQ(1, GetAdEventCount(AdType::kPromotedContentAd,
                               ConfirmationType::kServed));
  EXPECT_EQ(1, GetAdEventCount(AdType::kPromotedContentAd,
                               ConfirmationType::kViewed));
}

TEST_F(BatAdsPromotedContentAdEventHandlerTest,
       DoNotFireViewedEventIfAlreadyFired) {
  // Arrange
  ForcePermissionRulesForTesting();

  const CreativePromotedContentAdInfo creative_ad = BuildAndSaveCreativeAd();

  event_handler_->FireEvent(kPlacementId, creative_ad.creative_instance_id,
                            mojom::PromotedContentAdEventType::kServed);

  event_handler_->FireEvent(kPlacementId, creative_ad.creative_instance_id,
                            mojom::PromotedContentAdEventType::kViewed);

  // Act
  event_handler_->FireEvent(kPlacementId, creative_ad.creative_instance_id,
                            mojom::PromotedContentAdEventType::kViewed);

  // Assert
  EXPECT_EQ(1, GetAdEventCount(AdType::kPromotedContentAd,
                               ConfirmationType::kServed));
  EXPECT_EQ(1, GetAdEventCount(AdType::kPromotedContentAd,
                               ConfirmationType::kViewed));
}

TEST_F(BatAdsPromotedContentAdEventHandlerTest, FireClickedEvent) {
  // Arrange
  ForcePermissionRulesForTesting();

  const CreativePromotedContentAdInfo creative_ad = BuildAndSaveCreativeAd();

  event_handler_->FireEvent(kPlacementId, creative_ad.creative_instance_id,
                            mojom::PromotedContentAdEventType::kServed);

  event_handler_->FireEvent(kPlacementId, creative_ad.creative_instance_id,
                            mojom::PromotedContentAdEventType::kViewed);

  // Act
  event_handler_->FireEvent(kPlacementId, creative_ad.creative_instance_id,
                            mojom::PromotedContentAdEventType::kClicked);

  // Assert
  EXPECT_TRUE(did_serve_ad_);
  EXPECT_TRUE(did_view_ad_);
  EXPECT_TRUE(did_click_ad_);
  EXPECT_FALSE(did_fail_to_fire_event_);
  const PromotedContentAdInfo expected_ad =
      BuildPromotedContentAd(creative_ad, kPlacementId);
  EXPECT_EQ(expected_ad, ad_);
  EXPECT_EQ(1, GetAdEventCount(AdType::kPromotedContentAd,
                               ConfirmationType::kServed));
  EXPECT_EQ(1, GetAdEventCount(AdType::kPromotedContentAd,
                               ConfirmationType::kViewed));
  EXPECT_EQ(1, GetAdEventCount(AdType::kPromotedContentAd,
                               ConfirmationType::kClicked));
}

TEST_F(BatAdsPromotedContentAdEventHandlerTest,
       DoNotFireClickedEventIfMissingAdPlacement) {
  // Arrange
  const CreativePromotedContentAdInfo creative_ad = BuildAndSaveCreativeAd();

  // Act
  event_handler_->FireEvent(kPlacementId, creative_ad.creative_instance_id,
                            mojom::PromotedContentAdEventType::kClicked);

  // Assert
  EXPECT_EQ(0, GetAdEventCount(AdType::kPromotedContentAd,
                               ConfirmationType::kClicked));
}

TEST_F(BatAdsPromotedContentAdEventHandlerTest,
       DoNotFireClickedEventIfAlreadyFired) {
  // Arrange
  ForcePermissionRulesForTesting();

  const CreativePromotedContentAdInfo creative_ad = BuildAndSaveCreativeAd();

  event_handler_->FireEvent(kPlacementId, creative_ad.creative_instance_id,
                            mojom::PromotedContentAdEventType::kServed);
  event_handler_->FireEvent(kPlacementId, creative_ad.creative_instance_id,
                            mojom::PromotedContentAdEventType::kViewed);
  event_handler_->FireEvent(kPlacementId, creative_ad.creative_instance_id,
                            mojom::PromotedContentAdEventType::kClicked);

  // Act
  event_handler_->FireEvent(kPlacementId, creative_ad.creative_instance_id,
                            mojom::PromotedContentAdEventType::kClicked);

  // Assert
  EXPECT_EQ(1, GetAdEventCount(AdType::kPromotedContentAd,
                               ConfirmationType::kServed));
  EXPECT_EQ(1, GetAdEventCount(AdType::kPromotedContentAd,
                               ConfirmationType::kViewed));
  EXPECT_EQ(1, GetAdEventCount(AdType::kPromotedContentAd,
                               ConfirmationType::kClicked));
}

TEST_F(BatAdsPromotedContentAdEventHandlerTest,
       DoNotFireEventWithInvalidPlacementId) {
  // Arrange

  // Act
  event_handler_->FireEvent(kInvalidPlacementId, kCreativeInstanceId,
                            mojom::PromotedContentAdEventType::kServed);

  // Assert
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_FALSE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_TRUE(did_fail_to_fire_event_);
  EXPECT_EQ(0, GetAdEventCount(AdType::kPromotedContentAd,
                               ConfirmationType::kServed));
}

TEST_F(BatAdsPromotedContentAdEventHandlerTest,
       DoNotFireEventWithInvalidCreativeInstanceId) {
  // Arrange

  // Act
  event_handler_->FireEvent(kPlacementId, kInvalidCreativeInstanceId,
                            mojom::PromotedContentAdEventType::kViewed);

  // Assert
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_FALSE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_TRUE(did_fail_to_fire_event_);
  EXPECT_EQ(0, GetAdEventCount(AdType::kPromotedContentAd,
                               ConfirmationType::kViewed));
}

TEST_F(BatAdsPromotedContentAdEventHandlerTest,
       DoNotFireEventWhenNotPermitted) {
  // Arrange
  const CreativePromotedContentAdInfo creative_ad = BuildAndSaveCreativeAd();

  // Act
  event_handler_->FireEvent(kPlacementId, creative_ad.creative_instance_id,
                            mojom::PromotedContentAdEventType::kViewed);

  // Assert
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_FALSE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_TRUE(did_fail_to_fire_event_);
  EXPECT_EQ(0, GetAdEventCount(AdType::kPromotedContentAd,
                               ConfirmationType::kViewed));
}

TEST_F(BatAdsPromotedContentAdEventHandlerTest,
       DoNotFireEventForUnknownCreativeInstanceId) {
  // Arrange
  ForcePermissionRulesForTesting();

  // Act
  event_handler_->FireEvent(kPlacementId, kCreativeInstanceId,
                            mojom::PromotedContentAdEventType::kViewed);

  // Assert
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_FALSE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_TRUE(did_fail_to_fire_event_);
  EXPECT_EQ(0, GetAdEventCount(AdType::kPromotedContentAd,
                               ConfirmationType::kViewed));
}

TEST_F(BatAdsPromotedContentAdEventHandlerTest,
       FireEventIfNotExceededAdsPerHourCap) {
  // Arrange
  ForcePermissionRulesForTesting();

  const CreativePromotedContentAdInfo creative_ad = BuildAndSaveCreativeAd();
  const AdEventInfo ad_event =
      BuildAdEvent(creative_ad, AdType::kPromotedContentAd,
                   ConfirmationType::kServed, Now());

  const int ads_per_hour = features::GetMaximumPromotedContentAdsPerHour();

  FireAdEvents(ad_event, ads_per_hour - 1);

  const std::string placement_id =
      base::GUID::GenerateRandomV4().AsLowercaseString();

  // Act
  event_handler_->FireEvent(placement_id, creative_ad.creative_instance_id,
                            mojom::PromotedContentAdEventType::kServed);

  // Assert
  EXPECT_EQ(ads_per_hour, GetAdEventCount(AdType::kPromotedContentAd,
                                          ConfirmationType::kServed));
}

TEST_F(BatAdsPromotedContentAdEventHandlerTest,
       DoNotFireEventIfExceededAdsPerHourCap) {
  // Arrange
  ForcePermissionRulesForTesting();

  const CreativePromotedContentAdInfo creative_ad = BuildAndSaveCreativeAd();
  const AdEventInfo ad_event =
      BuildAdEvent(creative_ad, AdType::kPromotedContentAd,
                   ConfirmationType::kServed, Now());

  const int ads_per_hour = features::GetMaximumPromotedContentAdsPerHour();
  FireAdEvents(ad_event, ads_per_hour);

  const std::string placement_id =
      base::GUID::GenerateRandomV4().AsLowercaseString();

  // Act
  event_handler_->FireEvent(placement_id, creative_ad.creative_instance_id,
                            mojom::PromotedContentAdEventType::kServed);

  // Assert
  EXPECT_EQ(ads_per_hour, GetAdEventCount(AdType::kPromotedContentAd,
                                          ConfirmationType::kServed));
}

TEST_F(BatAdsPromotedContentAdEventHandlerTest,
       FireEventIfNotExceededAdsPerDayCap) {
  // Arrange
  ForcePermissionRulesForTesting();

  const CreativePromotedContentAdInfo creative_ad = BuildAndSaveCreativeAd();
  const AdEventInfo ad_event =
      BuildAdEvent(creative_ad, AdType::kPromotedContentAd,
                   ConfirmationType::kServed, Now());

  const int ads_per_day = features::GetMaximumPromotedContentAdsPerDay();

  FireAdEvents(ad_event, ads_per_day - 1);

  AdvanceClockBy(base::Hours(1));

  const std::string placement_id =
      base::GUID::GenerateRandomV4().AsLowercaseString();

  // Act
  event_handler_->FireEvent(placement_id, creative_ad.creative_instance_id,
                            mojom::PromotedContentAdEventType::kServed);

  // Assert
  EXPECT_EQ(ads_per_day, GetAdEventCount(AdType::kPromotedContentAd,
                                         ConfirmationType::kServed));
}

TEST_F(BatAdsPromotedContentAdEventHandlerTest,
       DoNotFireEventIfExceededAdsPerDayCap) {
  // Arrange
  ForcePermissionRulesForTesting();

  const CreativePromotedContentAdInfo creative_ad = BuildAndSaveCreativeAd();
  const AdEventInfo ad_event =
      BuildAdEvent(creative_ad, AdType::kPromotedContentAd,
                   ConfirmationType::kServed, Now());

  const int ads_per_day = features::GetMaximumPromotedContentAdsPerDay();

  FireAdEvents(ad_event, ads_per_day);

  AdvanceClockBy(base::Hours(1));

  const std::string placement_id =
      base::GUID::GenerateRandomV4().AsLowercaseString();

  // Act
  event_handler_->FireEvent(placement_id, creative_ad.creative_instance_id,
                            mojom::PromotedContentAdEventType::kServed);

  // Assert
  EXPECT_EQ(ads_per_day, GetAdEventCount(AdType::kPromotedContentAd,
                                         ConfirmationType::kServed));
}

}  // namespace ads::promoted_content_ads
