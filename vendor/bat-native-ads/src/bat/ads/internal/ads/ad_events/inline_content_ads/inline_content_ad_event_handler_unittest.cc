/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/ad_events/inline_content_ads/inline_content_ad_event_handler.h"

#include <memory>

#include "bat/ads/ad_type.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/inline_content_ad_info.h"
#include "bat/ads/internal/ads/ad_events/ad_event_unittest_util.h"
#include "bat/ads/internal/ads/ad_events/inline_content_ads/inline_content_ad_event_handler_observer.h"
#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/creatives/inline_content_ads/creative_inline_content_ad_info.h"
#include "bat/ads/internal/creatives/inline_content_ads/creative_inline_content_ad_unittest_util.h"
#include "bat/ads/internal/creatives/inline_content_ads/inline_content_ad_builder.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads::inline_content_ads {

namespace {

constexpr char kPlacementId[] = "d2ef9bb0-a0dc-472c-bc49-62105bb6da68";
constexpr char kInvalidPlacementId[] = "";

constexpr char kCreativeInstanceId[] = "1547f94f-9086-4db9-a441-efb2f0365269";
constexpr char kInvalidCreativeInstanceId[] = "";

CreativeInlineContentAdInfo BuildAndSaveCreativeAd() {
  CreativeInlineContentAdList creative_ads;
  CreativeInlineContentAdInfo creative_ad = BuildCreativeInlineContentAd();
  creative_ads.push_back(creative_ad);

  SaveCreativeAds(creative_ads);

  return creative_ad;
}

}  // namespace

class BatAdsInlineContentAdEventHandlerTest : public EventHandlerObserver,
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

  void OnInlineContentAdServed(const InlineContentAdInfo& ad) override {
    ad_ = ad;
    did_serve_ad_ = true;
  }

  void OnInlineContentAdViewed(const InlineContentAdInfo& ad) override {
    ad_ = ad;
    did_view_ad_ = true;
  }

  void OnInlineContentAdClicked(const InlineContentAdInfo& ad) override {
    ad_ = ad;
    did_click_ad_ = true;
  }

  void OnInlineContentAdEventFailed(
      const std::string& /*placement_id*/,
      const std::string& /*creative_instance_id*/,
      const mojom::InlineContentAdEventType /*event_type*/) override {
    did_fail_to_fire_event_ = true;
  }

  std::unique_ptr<EventHandler> event_handler_;

  InlineContentAdInfo ad_;
  bool did_serve_ad_ = false;
  bool did_view_ad_ = false;
  bool did_click_ad_ = false;
  bool did_fail_to_fire_event_ = false;
};

TEST_F(BatAdsInlineContentAdEventHandlerTest, FireViewedEvent) {
  // Arrange
  const CreativeInlineContentAdInfo creative_ad = BuildAndSaveCreativeAd();

  event_handler_->FireEvent(kPlacementId, creative_ad.creative_instance_id,
                            mojom::InlineContentAdEventType::kServed);

  // Act
  event_handler_->FireEvent(kPlacementId, creative_ad.creative_instance_id,
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
      1, GetAdEventCount(AdType::kInlineContentAd, ConfirmationType::kServed));
  EXPECT_EQ(
      1, GetAdEventCount(AdType::kInlineContentAd, ConfirmationType::kViewed));
}

TEST_F(BatAdsInlineContentAdEventHandlerTest,
       DoNotFireViewedEventIfAlreadyFired) {
  // Arrange
  const CreativeInlineContentAdInfo creative_ad = BuildAndSaveCreativeAd();

  event_handler_->FireEvent(kPlacementId, creative_ad.creative_instance_id,
                            mojom::InlineContentAdEventType::kServed);
  event_handler_->FireEvent(kPlacementId, creative_ad.creative_instance_id,
                            mojom::InlineContentAdEventType::kViewed);

  // Act
  event_handler_->FireEvent(kPlacementId, creative_ad.creative_instance_id,
                            mojom::InlineContentAdEventType::kViewed);

  // Assert
  EXPECT_EQ(
      1, GetAdEventCount(AdType::kInlineContentAd, ConfirmationType::kServed));
  EXPECT_EQ(
      1, GetAdEventCount(AdType::kInlineContentAd, ConfirmationType::kViewed));
}

TEST_F(BatAdsInlineContentAdEventHandlerTest, FireClickedEvent) {
  // Arrange
  const CreativeInlineContentAdInfo creative_ad = BuildAndSaveCreativeAd();

  event_handler_->FireEvent(kPlacementId, creative_ad.creative_instance_id,
                            mojom::InlineContentAdEventType::kServed);
  event_handler_->FireEvent(kPlacementId, creative_ad.creative_instance_id,
                            mojom::InlineContentAdEventType::kViewed);

  // Act
  event_handler_->FireEvent(kPlacementId, creative_ad.creative_instance_id,
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
      1, GetAdEventCount(AdType::kInlineContentAd, ConfirmationType::kServed));
  EXPECT_EQ(
      1, GetAdEventCount(AdType::kInlineContentAd, ConfirmationType::kViewed));
  EXPECT_EQ(
      1, GetAdEventCount(AdType::kInlineContentAd, ConfirmationType::kClicked));
}

TEST_F(BatAdsInlineContentAdEventHandlerTest,
       DoNotFireClickedEventIfMissingAdPlacement) {
  // Arrange
  const CreativeInlineContentAdInfo creative_ad = BuildAndSaveCreativeAd();

  // Act
  event_handler_->FireEvent(kPlacementId, creative_ad.creative_instance_id,
                            mojom::InlineContentAdEventType::kClicked);

  // Assert
  EXPECT_EQ(
      0, GetAdEventCount(AdType::kInlineContentAd, ConfirmationType::kClicked));
}

TEST_F(BatAdsInlineContentAdEventHandlerTest,
       DoNotFireClickedEventIfAlreadyFired) {
  // Arrange
  const CreativeInlineContentAdInfo creative_ad = BuildAndSaveCreativeAd();

  event_handler_->FireEvent(kPlacementId, creative_ad.creative_instance_id,
                            mojom::InlineContentAdEventType::kServed);
  event_handler_->FireEvent(kPlacementId, creative_ad.creative_instance_id,
                            mojom::InlineContentAdEventType::kViewed);
  event_handler_->FireEvent(kPlacementId, creative_ad.creative_instance_id,
                            mojom::InlineContentAdEventType::kClicked);

  // Act
  event_handler_->FireEvent(kPlacementId, creative_ad.creative_instance_id,
                            mojom::InlineContentAdEventType::kClicked);

  // Assert
  EXPECT_EQ(
      1, GetAdEventCount(AdType::kInlineContentAd, ConfirmationType::kServed));
  EXPECT_EQ(
      1, GetAdEventCount(AdType::kInlineContentAd, ConfirmationType::kViewed));
  EXPECT_EQ(
      1, GetAdEventCount(AdType::kInlineContentAd, ConfirmationType::kClicked));
}

TEST_F(BatAdsInlineContentAdEventHandlerTest,
       DoNotFireEventWithInvalidPlacementId) {
  // Arrange

  // Act
  event_handler_->FireEvent(kInvalidPlacementId, kCreativeInstanceId,
                            mojom::InlineContentAdEventType::kServed);

  // Assert
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_FALSE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_TRUE(did_fail_to_fire_event_);
  EXPECT_EQ(
      0, GetAdEventCount(AdType::kInlineContentAd, ConfirmationType::kServed));
}

TEST_F(BatAdsInlineContentAdEventHandlerTest,
       DoNotFireEventWithInvalidCreativeInstanceId) {
  // Arrange

  // Act
  event_handler_->FireEvent(kPlacementId, kInvalidCreativeInstanceId,
                            mojom::InlineContentAdEventType::kServed);

  // Assert
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_FALSE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_TRUE(did_fail_to_fire_event_);
  EXPECT_EQ(
      0, GetAdEventCount(AdType::kInlineContentAd, ConfirmationType::kServed));
}

TEST_F(BatAdsInlineContentAdEventHandlerTest,
       DoNotFireEventForUnknownCreativeInstanceId) {
  // Arrange

  // Act
  event_handler_->FireEvent(kPlacementId, kCreativeInstanceId,
                            mojom::InlineContentAdEventType::kServed);

  // Assert
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_FALSE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_TRUE(did_fail_to_fire_event_);
  EXPECT_EQ(
      0, GetAdEventCount(AdType::kInlineContentAd, ConfirmationType::kServed));
}

}  // namespace ads::inline_content_ads
