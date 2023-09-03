/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_interaction/ad_events/inline_content_ads/inline_content_ad_event_handler.h"

#include <vector>

#include "base/test/mock_callback.h"
#include "brave/components/brave_ads/core/internal/ads/ad_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/creative_inline_content_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/creative_inline_content_ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/creative_inline_content_ads_database_util.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/inline_content_ad_builder.h"
#include "brave/components/brave_ads/core/internal/user_interaction/ad_events/ad_event_unittest_util.h"
#include "brave/components/brave_ads/core/internal/user_interaction/ad_events/inline_content_ads/inline_content_ad_event_handler_delegate.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-shared.h"
#include "brave/components/brave_ads/core/public/ad_type.h"
#include "brave/components/brave_ads/core/public/ads/inline_content_ad_info.h"
#include "brave/components/brave_ads/core/public/confirmation_type.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

InlineContentAdInfo BuildAndSaveAd() {
  const CreativeInlineContentAdInfo creative_ad =
      BuildCreativeInlineContentAdForTesting(/*should_use_random_uuids*/ true);
  database::SaveCreativeInlineContentAds({creative_ad});
  return BuildInlineContentAd(creative_ad);
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

  void FireEvent(const std::string& placement_id,
                 const std::string& creative_instance_id,
                 const mojom::InlineContentAdEventType& event_type,
                 const bool should_fire_event) {
    base::MockCallback<FireInlineContentAdEventHandlerCallback> callback;
    EXPECT_CALL(callback,
                Run(/*success*/ should_fire_event, placement_id, event_type));

    event_handler_.FireEvent(placement_id, creative_instance_id, event_type,
                             callback.Get());
  }

  void FireEvents(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      const std::vector<mojom::InlineContentAdEventType>& event_types,
      const bool should_fire_event) {
    for (const auto& event_type : event_types) {
      FireEvent(placement_id, creative_instance_id, event_type,
                should_fire_event);
    }
  }

  InlineContentAdEventHandler event_handler_;

  InlineContentAdInfo ad_;
  bool did_serve_ad_ = false;
  bool did_view_ad_ = false;
  bool did_click_ad_ = false;
  bool did_fail_to_fire_event_ = false;
};

TEST_F(BraveAdsInlineContentAdEventHandlerTest, FireServedEvent) {
  // Arrange
  const InlineContentAdInfo ad = BuildAndSaveAd();

  // Act
  FireEvent(ad.placement_id, ad.creative_instance_id,
            mojom::InlineContentAdEventType::kServed,
            /*should_fire_event*/ true);

  // Assert
  EXPECT_TRUE(did_serve_ad_);
  EXPECT_FALSE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_FALSE(did_fail_to_fire_event_);
  EXPECT_EQ(ad, ad_);
  EXPECT_EQ(1U, GetAdEventCountForTesting(AdType::kInlineContentAd,
                                          ConfirmationType::kServed));
}

TEST_F(BraveAdsInlineContentAdEventHandlerTest, FireViewedEvent) {
  // Arrange
  const InlineContentAdInfo ad = BuildAndSaveAd();

  FireEvent(ad.placement_id, ad.creative_instance_id,
            mojom::InlineContentAdEventType::kServed,
            /*should_fire_event*/ true);

  // Act
  FireEvent(ad.placement_id, ad.creative_instance_id,
            mojom::InlineContentAdEventType::kViewed,
            /*should_fire_event*/ true);

  // Assert
  EXPECT_TRUE(did_serve_ad_);
  EXPECT_TRUE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_FALSE(did_fail_to_fire_event_);
  EXPECT_EQ(ad, ad_);
  EXPECT_EQ(1U, GetAdEventCountForTesting(AdType::kInlineContentAd,
                                          ConfirmationType::kServed));
  EXPECT_EQ(1U, GetAdEventCountForTesting(AdType::kInlineContentAd,
                                          ConfirmationType::kViewed));
}

TEST_F(BraveAdsInlineContentAdEventHandlerTest,
       DoNotFireViewedEventIfAdPlacementWasAlreadyViewed) {
  // Arrange
  const InlineContentAdInfo ad = BuildAndSaveAd();

  FireEvents(ad.placement_id, ad.creative_instance_id,
             {mojom::InlineContentAdEventType::kServed,
              mojom::InlineContentAdEventType::kViewed},
             /*should_fire_event*/ true);

  // Act
  FireEvent(ad.placement_id, ad.creative_instance_id,
            mojom::InlineContentAdEventType::kViewed,
            /*should_fire_event*/ false);

  // Assert
  EXPECT_EQ(1U, GetAdEventCountForTesting(AdType::kInlineContentAd,
                                          ConfirmationType::kServed));
  EXPECT_EQ(1U, GetAdEventCountForTesting(AdType::kInlineContentAd,
                                          ConfirmationType::kViewed));
}

TEST_F(BraveAdsInlineContentAdEventHandlerTest,
       DoNotFireViewedEventIfAdPlacementWasNotServed) {
  // Arrange
  const InlineContentAdInfo ad = BuildAndSaveAd();

  // Act
  FireEvent(ad.placement_id, ad.creative_instance_id,
            mojom::InlineContentAdEventType::kViewed,
            /*should_fire_event*/ false);

  // Assert
  EXPECT_EQ(0U, GetAdEventCountForTesting(AdType::kInlineContentAd,
                                          ConfirmationType::kServed));
  EXPECT_EQ(0U, GetAdEventCountForTesting(AdType::kInlineContentAd,
                                          ConfirmationType::kViewed));
}

TEST_F(BraveAdsInlineContentAdEventHandlerTest, FireClickedEvent) {
  // Arrange
  const InlineContentAdInfo ad = BuildAndSaveAd();

  FireEvents(ad.placement_id, ad.creative_instance_id,
             {mojom::InlineContentAdEventType::kServed,
              mojom::InlineContentAdEventType::kViewed},
             /*should_fire_event*/ true);

  // Act
  FireEvent(ad.placement_id, ad.creative_instance_id,
            mojom::InlineContentAdEventType::kClicked,
            /*should_fire_event*/ true);

  // Assert
  EXPECT_TRUE(did_serve_ad_);
  EXPECT_TRUE(did_view_ad_);
  EXPECT_TRUE(did_click_ad_);
  EXPECT_FALSE(did_fail_to_fire_event_);
  EXPECT_EQ(ad, ad_);
  EXPECT_EQ(1U, GetAdEventCountForTesting(AdType::kInlineContentAd,
                                          ConfirmationType::kServed));
  EXPECT_EQ(1U, GetAdEventCountForTesting(AdType::kInlineContentAd,
                                          ConfirmationType::kViewed));
  EXPECT_EQ(1U, GetAdEventCountForTesting(AdType::kInlineContentAd,
                                          ConfirmationType::kClicked));
}

TEST_F(BraveAdsInlineContentAdEventHandlerTest,
       DoNotFireClickedEventIfAdPlacementWasAlreadyClicked) {
  // Arrange
  const InlineContentAdInfo ad = BuildAndSaveAd();

  FireEvents(ad.placement_id, ad.creative_instance_id,
             {mojom::InlineContentAdEventType::kServed,
              mojom::InlineContentAdEventType::kViewed,
              mojom::InlineContentAdEventType::kClicked},
             /*should_fire_event*/ true);

  // Act
  FireEvent(ad.placement_id, ad.creative_instance_id,
            mojom::InlineContentAdEventType::kClicked,
            /*should_fire_event*/ false);

  // Assert
  EXPECT_EQ(1U, GetAdEventCountForTesting(AdType::kInlineContentAd,
                                          ConfirmationType::kServed));
  EXPECT_EQ(1U, GetAdEventCountForTesting(AdType::kInlineContentAd,
                                          ConfirmationType::kViewed));
  EXPECT_EQ(1U, GetAdEventCountForTesting(AdType::kInlineContentAd,
                                          ConfirmationType::kClicked));
}

TEST_F(BraveAdsInlineContentAdEventHandlerTest,
       DoNotFireClickedEventIfAdPlacementWasNotServed) {
  // Arrange
  const InlineContentAdInfo ad = BuildAndSaveAd();

  // Act
  FireEvent(ad.placement_id, ad.creative_instance_id,
            mojom::InlineContentAdEventType::kViewed,
            /*should_fire_event*/ false);

  // Assert
  EXPECT_EQ(0U, GetAdEventCountForTesting(AdType::kInlineContentAd,
                                          ConfirmationType::kServed));
  EXPECT_EQ(0U, GetAdEventCountForTesting(AdType::kInlineContentAd,
                                          ConfirmationType::kViewed));
}

TEST_F(BraveAdsInlineContentAdEventHandlerTest,
       DoNotFireEventWithInvalidPlacementId) {
  // Arrange

  // Act
  FireEvent(kInvalidPlacementId, kCreativeInstanceId,
            mojom::InlineContentAdEventType::kServed,
            /*should_fire_event*/ false);

  // Assert
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_FALSE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_TRUE(did_fail_to_fire_event_);
  EXPECT_EQ(0U, GetAdEventCountForTesting(AdType::kInlineContentAd,
                                          ConfirmationType::kServed));
}

TEST_F(BraveAdsInlineContentAdEventHandlerTest,
       DoNotFireEventWithInvalidCreativeInstanceId) {
  // Arrange

  // Act
  FireEvent(kPlacementId, kInvalidCreativeInstanceId,
            mojom::InlineContentAdEventType::kServed,
            /*should_fire_event*/ false);

  // Assert
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_FALSE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_TRUE(did_fail_to_fire_event_);
  EXPECT_EQ(0U, GetAdEventCountForTesting(AdType::kInlineContentAd,
                                          ConfirmationType::kServed));
}

TEST_F(BraveAdsInlineContentAdEventHandlerTest,
       DoNotFireEventForMissingCreativeInstanceId) {
  // Arrange
  const InlineContentAdInfo ad = BuildAndSaveAd();

  // Act
  FireEvent(ad.placement_id, kMissingCreativeInstanceId,
            mojom::InlineContentAdEventType::kServed,
            /*should_fire_event*/ false);

  // Assert
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_FALSE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_TRUE(did_fail_to_fire_event_);
  EXPECT_EQ(0U, GetAdEventCountForTesting(AdType::kInlineContentAd,
                                          ConfirmationType::kServed));
}

}  // namespace brave_ads
