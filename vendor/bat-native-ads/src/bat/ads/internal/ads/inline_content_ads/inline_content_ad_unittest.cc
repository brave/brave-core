/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/inline_content_ads/inline_content_ad.h"

#include <memory>

#include "bat/ads/ad_type.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/inline_content_ad_info.h"
#include "bat/ads/internal/ad_events/ad_event_unittest_util.h"
#include "bat/ads/internal/ads/inline_content_ads/inline_content_ad_builder.h"
#include "bat/ads/internal/ads/inline_content_ads/inline_content_ad_observer.h"
#include "bat/ads/internal/bundle/creative_inline_content_ad_info.h"
#include "bat/ads/internal/bundle/creative_inline_content_ad_unittest_util.h"
#include "bat/ads/internal/database/tables/ad_events_database_table.h"
#include "bat/ads/internal/features/ad_serving/ad_serving_features.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"
#include "bat/ads/public/interfaces/ads.mojom.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {

constexpr char kUuid[] = "d2ef9bb0-a0dc-472c-bc49-62105bb6da68";
constexpr char kInvalidUuid[] = "";

constexpr char kCreativeInstanceId[] = "1547f94f-9086-4db9-a441-efb2f0365269";
constexpr char kInvalidCreativeInstanceId[] = "";

}  // namespace

class BatAdsInlineContentAdTest : public InlineContentAdObserver,
                                  public UnitTestBase {
 protected:
  BatAdsInlineContentAdTest()
      : inline_content_ad_(std::make_unique<InlineContentAd>()) {
    inline_content_ad_->AddObserver(this);
  }

  ~BatAdsInlineContentAdTest() override = default;

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
      const std::string& uuid,
      const std::string& creative_instance_id,
      const mojom::InlineContentAdEventType event_type) override {
    did_fail_to_fire_event_ = true;
  }

  CreativeInlineContentAdInfo BuildAndSaveCreativeAd() {
    CreativeInlineContentAdList creative_ads;
    const CreativeInlineContentAdInfo& creative_ad =
        BuildCreativeInlineContentAd();
    creative_ads.push_back(creative_ad);

    SaveCreativeAds(creative_ads);

    return creative_ad;
  }

  void ExpectAdEventCountEquals(const ConfirmationType& confirmation_type,
                                const int expected_count) {
    database::table::AdEvents database_table;
    database_table.GetAll(
        [=](const bool success, const AdEventList& ad_events) {
          ASSERT_TRUE(success);

          const int count = GetAdEventCount(AdType::kInlineContentAd,
                                            confirmation_type, ad_events);
          EXPECT_EQ(expected_count, count);
        });
  }

  std::unique_ptr<InlineContentAd> inline_content_ad_;

  InlineContentAdInfo ad_;
  bool did_serve_ad_ = false;
  bool did_view_ad_ = false;
  bool did_click_ad_ = false;
  bool did_fail_to_fire_event_ = false;
};

TEST_F(BatAdsInlineContentAdTest, FireViewedEvent) {
  // Arrange
  const CreativeInlineContentAdInfo& creative_ad = BuildAndSaveCreativeAd();

  // Act
  inline_content_ad_->FireEvent(kUuid, creative_ad.creative_instance_id,
                                mojom::InlineContentAdEventType::kViewed);

  // Assert
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_TRUE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_FALSE(did_fail_to_fire_event_);
  const InlineContentAdInfo& expected_ad =
      BuildInlineContentAd(creative_ad, kUuid);
  EXPECT_EQ(expected_ad, ad_);

  ExpectAdEventCountEquals(ConfirmationType::kViewed, 1);
}

TEST_F(BatAdsInlineContentAdTest, FireClickedEvent) {
  // Arrange
  const CreativeInlineContentAdInfo& creative_ad = BuildAndSaveCreativeAd();

  // Act
  inline_content_ad_->FireEvent(kUuid, creative_ad.creative_instance_id,
                                mojom::InlineContentAdEventType::kClicked);

  // Assert
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_FALSE(did_view_ad_);
  EXPECT_TRUE(did_click_ad_);
  EXPECT_FALSE(did_fail_to_fire_event_);
  const InlineContentAdInfo& expected_ad =
      BuildInlineContentAd(creative_ad, kUuid);
  EXPECT_EQ(expected_ad, ad_);

  ExpectAdEventCountEquals(ConfirmationType::kClicked, 1);
}

TEST_F(BatAdsInlineContentAdTest, DoNotFireViewedEventIfAlreadyFired) {
  // Arrange
  const CreativeInlineContentAdInfo& creative_ad = BuildAndSaveCreativeAd();

  inline_content_ad_->FireEvent(kUuid, creative_ad.creative_instance_id,
                                mojom::InlineContentAdEventType::kViewed);

  // Act
  inline_content_ad_->FireEvent(kUuid, creative_ad.creative_instance_id,
                                mojom::InlineContentAdEventType::kViewed);

  // Assert
  ExpectAdEventCountEquals(ConfirmationType::kViewed, 1);
}

TEST_F(BatAdsInlineContentAdTest, DoNotFireEventWithInvalidUuid) {
  // Arrange

  // Act
  inline_content_ad_->FireEvent(kInvalidUuid, kCreativeInstanceId,
                                mojom::InlineContentAdEventType::kViewed);

  // Assert
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_FALSE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_TRUE(did_fail_to_fire_event_);

  ExpectAdEventCountEquals(ConfirmationType::kViewed, 0);
}

TEST_F(BatAdsInlineContentAdTest, DoNotFireEventWithInvalidCreativeInstanceId) {
  // Arrange

  // Act
  inline_content_ad_->FireEvent(kUuid, kInvalidCreativeInstanceId,
                                mojom::InlineContentAdEventType::kViewed);

  // Assert
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_FALSE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_TRUE(did_fail_to_fire_event_);

  ExpectAdEventCountEquals(ConfirmationType::kViewed, 0);
}

TEST_F(BatAdsInlineContentAdTest,
       DoNotFireEventIfCreativeInstanceIdWasNotFound) {
  // Arrange

  // Act
  inline_content_ad_->FireEvent(kUuid, kCreativeInstanceId,
                                mojom::InlineContentAdEventType::kViewed);

  // Assert
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_FALSE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_TRUE(did_fail_to_fire_event_);

  ExpectAdEventCountEquals(ConfirmationType::kViewed, 0);
}

}  // namespace ads
