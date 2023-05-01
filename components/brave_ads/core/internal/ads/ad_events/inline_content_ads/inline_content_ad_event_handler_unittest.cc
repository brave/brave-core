/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/ad_events/inline_content_ads/inline_content_ad_event_handler.h"

#include "brave/components/brave_ads/core/ad_type.h"
#include "brave/components/brave_ads/core/confirmation_type.h"
#include "brave/components/brave_ads/core/inline_content_ad_info.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/ad_event_unittest_util.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/inline_content_ads/inline_content_ad_event_handler_delegate.h"
#include "brave/components/brave_ads/core/internal/ads/ad_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/creative_inline_content_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/creative_inline_content_ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/inline_content_ad_builder.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

CreativeInlineContentAdInfo BuildAndSaveCreativeAd() {
  CreativeInlineContentAdInfo creative_ad =
      BuildCreativeInlineContentAd(/*should_use_random_guids*/ true);

  SaveCreativeAds({creative_ad});

  return creative_ad;
}

}  // namespace

class BraveAdsInlineContentAdEventHandlerTest
    : public InlineContentAdEventHandlerDelegate,
      public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    event_handler_.SetDelegate(this);
  }

  void OnDidFireInlineContentAdServedEvent(
      const InlineContentAdInfo& ad) override {
    ad_ = ad;
    did_serve_ad_ = true;
  }

  void OnDidFireInlineContentAdViewedEvent(
      const InlineContentAdInfo& ad) override {
    ad_ = ad;
    did_view_ad_ = true;
  }

  void OnDidFireInlineContentAdClickedEvent(
      const InlineContentAdInfo& ad) override {
    ad_ = ad;
    did_click_ad_ = true;
  }

  void OnFailedToFireInlineContentAdEvent(
      const std::string& /*placement_id*/,
      const std::string& /*creative_instance_id*/,
      const mojom::InlineContentAdEventType /*event_type*/) override {
    did_fail_to_fire_event_ = true;
  }

  InlineContentAdEventHandler event_handler_;

  InlineContentAdInfo ad_;
  bool did_serve_ad_ = false;
  bool did_view_ad_ = false;
  bool did_click_ad_ = false;
  bool did_fail_to_fire_event_ = false;
};

TEST_F(BraveAdsInlineContentAdEventHandlerTest, FireViewedEvent) {
  // Arrange
  const CreativeInlineContentAdInfo creative_ad = BuildAndSaveCreativeAd();

  event_handler_.FireEvent(kPlacementId, creative_ad.creative_instance_id,
                           mojom::InlineContentAdEventType::kServed);

  // Act
  event_handler_.FireEvent(kPlacementId, creative_ad.creative_instance_id,
                           mojom::InlineContentAdEventType::kViewed);

  // Assert
  EXPECT_TRUE(did_serve_ad_);
  EXPECT_TRUE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_FALSE(did_fail_to_fire_event_);
  const InlineContentAdInfo expected_ad =
      BuildInlineContentAd(creative_ad, kPlacementId);
  EXPECT_EQ(expected_ad, ad_);
  EXPECT_EQ(
      1U, GetAdEventCount(AdType::kInlineContentAd, ConfirmationType::kServed));
  EXPECT_EQ(
      1U, GetAdEventCount(AdType::kInlineContentAd, ConfirmationType::kViewed));
}

TEST_F(BraveAdsInlineContentAdEventHandlerTest,
       DoNotFireViewedEventIfAlreadyFired) {
  // Arrange
  const CreativeInlineContentAdInfo creative_ad = BuildAndSaveCreativeAd();

  event_handler_.FireEvent(kPlacementId, creative_ad.creative_instance_id,
                           mojom::InlineContentAdEventType::kServed);
  event_handler_.FireEvent(kPlacementId, creative_ad.creative_instance_id,
                           mojom::InlineContentAdEventType::kViewed);

  // Act
  event_handler_.FireEvent(kPlacementId, creative_ad.creative_instance_id,
                           mojom::InlineContentAdEventType::kViewed);

  // Assert
  EXPECT_EQ(
      1U, GetAdEventCount(AdType::kInlineContentAd, ConfirmationType::kServed));
  EXPECT_EQ(
      1U, GetAdEventCount(AdType::kInlineContentAd, ConfirmationType::kViewed));
}

TEST_F(BraveAdsInlineContentAdEventHandlerTest, FireClickedEvent) {
  // Arrange
  const CreativeInlineContentAdInfo creative_ad = BuildAndSaveCreativeAd();

  event_handler_.FireEvent(kPlacementId, creative_ad.creative_instance_id,
                           mojom::InlineContentAdEventType::kServed);
  event_handler_.FireEvent(kPlacementId, creative_ad.creative_instance_id,
                           mojom::InlineContentAdEventType::kViewed);

  // Act
  event_handler_.FireEvent(kPlacementId, creative_ad.creative_instance_id,
                           mojom::InlineContentAdEventType::kClicked);

  // Assert
  EXPECT_TRUE(did_serve_ad_);
  EXPECT_TRUE(did_view_ad_);
  EXPECT_TRUE(did_click_ad_);
  EXPECT_FALSE(did_fail_to_fire_event_);
  const InlineContentAdInfo expected_ad =
      BuildInlineContentAd(creative_ad, kPlacementId);
  EXPECT_EQ(expected_ad, ad_);
  EXPECT_EQ(
      1U, GetAdEventCount(AdType::kInlineContentAd, ConfirmationType::kServed));
  EXPECT_EQ(
      1U, GetAdEventCount(AdType::kInlineContentAd, ConfirmationType::kViewed));
  EXPECT_EQ(1U, GetAdEventCount(AdType::kInlineContentAd,
                                ConfirmationType::kClicked));
}

TEST_F(BraveAdsInlineContentAdEventHandlerTest,
       DoNotFireClickedEventIfMissingAdPlacement) {
  // Arrange
  const CreativeInlineContentAdInfo creative_ad = BuildAndSaveCreativeAd();

  // Act
  event_handler_.FireEvent(kPlacementId, creative_ad.creative_instance_id,
                           mojom::InlineContentAdEventType::kClicked);

  // Assert
  EXPECT_EQ(0U, GetAdEventCount(AdType::kInlineContentAd,
                                ConfirmationType::kClicked));
}

TEST_F(BraveAdsInlineContentAdEventHandlerTest,
       DoNotFireClickedEventIfAlreadyFired) {
  // Arrange
  const CreativeInlineContentAdInfo creative_ad = BuildAndSaveCreativeAd();

  event_handler_.FireEvent(kPlacementId, creative_ad.creative_instance_id,
                           mojom::InlineContentAdEventType::kServed);
  event_handler_.FireEvent(kPlacementId, creative_ad.creative_instance_id,
                           mojom::InlineContentAdEventType::kViewed);
  event_handler_.FireEvent(kPlacementId, creative_ad.creative_instance_id,
                           mojom::InlineContentAdEventType::kClicked);

  // Act
  event_handler_.FireEvent(kPlacementId, creative_ad.creative_instance_id,
                           mojom::InlineContentAdEventType::kClicked);

  // Assert
  EXPECT_EQ(
      1U, GetAdEventCount(AdType::kInlineContentAd, ConfirmationType::kServed));
  EXPECT_EQ(
      1U, GetAdEventCount(AdType::kInlineContentAd, ConfirmationType::kViewed));
  EXPECT_EQ(1U, GetAdEventCount(AdType::kInlineContentAd,
                                ConfirmationType::kClicked));
}

TEST_F(BraveAdsInlineContentAdEventHandlerTest,
       DoNotFireEventWithInvalidPlacementId) {
  // Arrange

  // Act
  event_handler_.FireEvent(kInvalidPlacementId, kCreativeInstanceId,
                           mojom::InlineContentAdEventType::kServed);

  // Assert
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_FALSE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_TRUE(did_fail_to_fire_event_);
  EXPECT_EQ(
      0U, GetAdEventCount(AdType::kInlineContentAd, ConfirmationType::kServed));
}

TEST_F(BraveAdsInlineContentAdEventHandlerTest,
       DoNotFireEventWithInvalidCreativeInstanceId) {
  // Arrange

  // Act
  event_handler_.FireEvent(kPlacementId, kInvalidCreativeInstanceId,
                           mojom::InlineContentAdEventType::kServed);

  // Assert
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_FALSE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_TRUE(did_fail_to_fire_event_);
  EXPECT_EQ(
      0U, GetAdEventCount(AdType::kInlineContentAd, ConfirmationType::kServed));
}

TEST_F(BraveAdsInlineContentAdEventHandlerTest,
       DoNotFireEventForUnknownCreativeInstanceId) {
  // Arrange

  // Act
  event_handler_.FireEvent(kPlacementId, kCreativeInstanceId,
                           mojom::InlineContentAdEventType::kServed);

  // Assert
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_FALSE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_TRUE(did_fail_to_fire_event_);
  EXPECT_EQ(
      0U, GetAdEventCount(AdType::kInlineContentAd, ConfirmationType::kServed));
}

}  // namespace brave_ads
