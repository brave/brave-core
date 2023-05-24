/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/ad_events/new_tab_page_ads/new_tab_page_ad_event_handler.h"

#include "base/guid.h"
#include "brave/components/brave_ads/core/ad_type.h"
#include "brave/components/brave_ads/core/confirmation_type.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/ad_event_info.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/ad_event_unittest_util.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/new_tab_page_ads/new_tab_page_ad_event_handler_delegate.h"
#include "brave/components/brave_ads/core/internal/ads/ad_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/ads/ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/ads/new_tab_page_ad_feature.h"
#include "brave/components/brave_ads/core/internal/ads/serving/permission_rules/permission_rules_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ads_database_util.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/new_tab_page_ad_builder.h"
#include "brave/components/brave_ads/core/new_tab_page_ad_info.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

CreativeNewTabPageAdInfo BuildAndSaveCreativeAd() {
  CreativeNewTabPageAdInfo creative_ad =
      BuildCreativeNewTabPageAd(/*should_use_random_guids*/ true);

  database::SaveCreativeNewTabPageAds({creative_ad});

  return creative_ad;
}

}  // namespace

class BraveAdsNewTabPageAdEventHandlerIfAdsDisabledTest
    : public NewTabPageAdEventHandlerDelegate,
      public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    DisableBravePrivateAds();

    event_handler_.SetDelegate(this);
  }

  void OnDidFireNewTabPageAdServedEvent(const NewTabPageAdInfo& ad) override {
    ad_ = ad;
    did_serve_ad_ = true;
  }

  void OnDidFireNewTabPageAdViewedEvent(const NewTabPageAdInfo& ad) override {
    ad_ = ad;
    did_view_ad_ = true;
  }

  void OnDidFireNewTabPageAdClickedEvent(const NewTabPageAdInfo& ad) override {
    ad_ = ad;
    did_click_ad_ = true;
  }

  void OnFailedToFireNewTabPageAdEvent(
      const std::string& /*placement_id*/,
      const std::string& /*creative_instance_id*/,
      const mojom::NewTabPageAdEventType /*event_type*/) override {
    did_fail_to_fire_event_ = true;
  }

  NewTabPageAdEventHandler event_handler_;

  NewTabPageAdInfo ad_;
  bool did_serve_ad_ = false;
  bool did_view_ad_ = false;
  bool did_click_ad_ = false;
  bool did_fail_to_fire_event_ = false;
};

TEST_F(BraveAdsNewTabPageAdEventHandlerIfAdsDisabledTest, FireViewedEvent) {
  // Arrange
  ForcePermissionRulesForTesting();

  const CreativeNewTabPageAdInfo creative_ad = BuildAndSaveCreativeAd();

  event_handler_.FireEvent(kPlacementId, creative_ad.creative_instance_id,
                           mojom::NewTabPageAdEventType::kServed);

  // Act
  event_handler_.FireEvent(kPlacementId, creative_ad.creative_instance_id,
                           mojom::NewTabPageAdEventType::kViewed);

  // Assert
  EXPECT_TRUE(did_serve_ad_);
  EXPECT_TRUE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_FALSE(did_fail_to_fire_event_);
  const NewTabPageAdInfo expected_ad =
      BuildNewTabPageAd(creative_ad, kPlacementId);
  EXPECT_EQ(expected_ad, ad_);
  EXPECT_EQ(1U,
            GetAdEventCount(AdType::kNewTabPageAd, ConfirmationType::kServed));
  EXPECT_EQ(1U,
            GetAdEventCount(AdType::kNewTabPageAd, ConfirmationType::kViewed));
}

TEST_F(BraveAdsNewTabPageAdEventHandlerIfAdsDisabledTest, FireClickedEvent) {
  // Arrange
  ForcePermissionRulesForTesting();

  const CreativeNewTabPageAdInfo creative_ad = BuildAndSaveCreativeAd();

  event_handler_.FireEvent(kPlacementId, creative_ad.creative_instance_id,
                           mojom::NewTabPageAdEventType::kServed);

  event_handler_.FireEvent(kPlacementId, creative_ad.creative_instance_id,
                           mojom::NewTabPageAdEventType::kViewed);

  // Act
  event_handler_.FireEvent(kPlacementId, creative_ad.creative_instance_id,
                           mojom::NewTabPageAdEventType::kClicked);

  // Assert
  EXPECT_TRUE(did_serve_ad_);
  EXPECT_TRUE(did_view_ad_);
  EXPECT_TRUE(did_click_ad_);
  EXPECT_FALSE(did_fail_to_fire_event_);
  const NewTabPageAdInfo expected_ad =
      BuildNewTabPageAd(creative_ad, kPlacementId);
  EXPECT_EQ(expected_ad, ad_);
  EXPECT_EQ(1U,
            GetAdEventCount(AdType::kNewTabPageAd, ConfirmationType::kServed));
  EXPECT_EQ(1U,
            GetAdEventCount(AdType::kNewTabPageAd, ConfirmationType::kViewed));
  EXPECT_EQ(1U,
            GetAdEventCount(AdType::kNewTabPageAd, ConfirmationType::kClicked));
}

TEST_F(BraveAdsNewTabPageAdEventHandlerIfAdsDisabledTest,
       DoNotFireViewedEventIfAlreadyFired) {
  // Arrange
  ForcePermissionRulesForTesting();

  const CreativeNewTabPageAdInfo creative_ad = BuildAndSaveCreativeAd();

  event_handler_.FireEvent(kPlacementId, creative_ad.creative_instance_id,
                           mojom::NewTabPageAdEventType::kServed);

  // Act
  event_handler_.FireEvent(kPlacementId, creative_ad.creative_instance_id,
                           mojom::NewTabPageAdEventType::kViewed);

  // Assert
  EXPECT_EQ(1U,
            GetAdEventCount(AdType::kNewTabPageAd, ConfirmationType::kServed));
  EXPECT_EQ(1U,
            GetAdEventCount(AdType::kNewTabPageAd, ConfirmationType::kViewed));
}

TEST_F(BraveAdsNewTabPageAdEventHandlerIfAdsDisabledTest,
       DoNotFireEventWithInvalidPlacementId) {
  // Arrange

  // Act
  event_handler_.FireEvent(kInvalidPlacementId, kCreativeInstanceId,
                           mojom::NewTabPageAdEventType::kViewed);

  // Assert
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_FALSE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_TRUE(did_fail_to_fire_event_);
  EXPECT_EQ(0U,
            GetAdEventCount(AdType::kNewTabPageAd, ConfirmationType::kServed));
  EXPECT_EQ(0U,
            GetAdEventCount(AdType::kNewTabPageAd, ConfirmationType::kViewed));
}

TEST_F(BraveAdsNewTabPageAdEventHandlerIfAdsDisabledTest,
       DoNotFireEventWithInvalidCreativeInstanceId) {
  // Arrange

  // Act
  event_handler_.FireEvent(kPlacementId, kInvalidCreativeInstanceId,
                           mojom::NewTabPageAdEventType::kViewed);

  // Assert
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_FALSE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_TRUE(did_fail_to_fire_event_);
  EXPECT_EQ(0U,
            GetAdEventCount(AdType::kNewTabPageAd, ConfirmationType::kServed));
  EXPECT_EQ(0U,
            GetAdEventCount(AdType::kNewTabPageAd, ConfirmationType::kViewed));
}

TEST_F(BraveAdsNewTabPageAdEventHandlerIfAdsDisabledTest,
       DoNotFireEventWhenNotPermitted) {
  // Arrange
  const CreativeNewTabPageAdInfo creative_ad = BuildAndSaveCreativeAd();

  // Act
  event_handler_.FireEvent(kPlacementId, creative_ad.creative_instance_id,
                           mojom::NewTabPageAdEventType::kViewed);

  // Assert
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_FALSE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_TRUE(did_fail_to_fire_event_);
  EXPECT_EQ(0U,
            GetAdEventCount(AdType::kNewTabPageAd, ConfirmationType::kServed));
  EXPECT_EQ(0U,
            GetAdEventCount(AdType::kNewTabPageAd, ConfirmationType::kViewed));
}

TEST_F(BraveAdsNewTabPageAdEventHandlerIfAdsDisabledTest,
       DoNotFireEventForUnknownCreativeInstanceId) {
  // Arrange
  ForcePermissionRulesForTesting();

  // Act
  event_handler_.FireEvent(kPlacementId, kCreativeInstanceId,
                           mojom::NewTabPageAdEventType::kViewed);

  // Assert
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_FALSE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_TRUE(did_fail_to_fire_event_);
  EXPECT_EQ(0U,
            GetAdEventCount(AdType::kNewTabPageAd, ConfirmationType::kServed));
  EXPECT_EQ(0U,
            GetAdEventCount(AdType::kNewTabPageAd, ConfirmationType::kViewed));
}

TEST_F(BraveAdsNewTabPageAdEventHandlerIfAdsDisabledTest,
       FireEventIfNotExceededAdsPerHourCap) {
  // Arrange
  ForcePermissionRulesForTesting();

  const std::string placement_id =
      base::GUID::GenerateRandomV4().AsLowercaseString();

  const size_t ads_per_hour = kMaximumNewTabPageAdsPerHour.Get();

  const CreativeNewTabPageAdInfo creative_ad = BuildAndSaveCreativeAd();
  const AdEventInfo served_ad_event = BuildAdEvent(
      creative_ad, AdType::kNewTabPageAd, ConfirmationType::kServed, Now());
  FireAdEvents(served_ad_event, ads_per_hour - 1);
  const AdEventInfo viewed_ad_event = BuildAdEvent(
      creative_ad, AdType::kNewTabPageAd, ConfirmationType::kViewed, Now());
  FireAdEvents(viewed_ad_event, ads_per_hour - 1);

  AdvanceClockBy(kNewTabPageAdMinimumWaitTime.Get());

  event_handler_.FireEvent(placement_id, creative_ad.creative_instance_id,
                           mojom::NewTabPageAdEventType::kServed);

  // Act
  event_handler_.FireEvent(placement_id, creative_ad.creative_instance_id,
                           mojom::NewTabPageAdEventType::kViewed);

  // Assert
  EXPECT_EQ(ads_per_hour,
            GetAdEventCount(AdType::kNewTabPageAd, ConfirmationType::kServed));
  EXPECT_EQ(ads_per_hour,
            GetAdEventCount(AdType::kNewTabPageAd, ConfirmationType::kViewed));
}

TEST_F(BraveAdsNewTabPageAdEventHandlerIfAdsDisabledTest,
       DoNotFireEventIfExceededAdsPerHourCap) {
  // Arrange
  ForcePermissionRulesForTesting();

  const size_t ads_per_hour = kMaximumNewTabPageAdsPerHour.Get();

  const CreativeNewTabPageAdInfo creative_ad = BuildAndSaveCreativeAd();
  const AdEventInfo served_ad_event = BuildAdEvent(
      creative_ad, AdType::kNewTabPageAd, ConfirmationType::kServed, Now());
  FireAdEvents(served_ad_event, ads_per_hour);
  const AdEventInfo viewed_ad_event = BuildAdEvent(
      creative_ad, AdType::kNewTabPageAd, ConfirmationType::kViewed, Now());
  FireAdEvents(viewed_ad_event, ads_per_hour);

  const std::string placement_id =
      base::GUID::GenerateRandomV4().AsLowercaseString();

  // Act
  event_handler_.FireEvent(placement_id, creative_ad.creative_instance_id,
                           mojom::NewTabPageAdEventType::kViewed);

  // Assert
  EXPECT_EQ(ads_per_hour,
            GetAdEventCount(AdType::kNewTabPageAd, ConfirmationType::kServed));
  EXPECT_EQ(ads_per_hour,
            GetAdEventCount(AdType::kNewTabPageAd, ConfirmationType::kViewed));
}

TEST_F(BraveAdsNewTabPageAdEventHandlerIfAdsDisabledTest,
       FireEventIfNotExceededAdsPerDayCap) {
  // Arrange
  ForcePermissionRulesForTesting();

  const size_t ads_per_day = kMaximumNewTabPageAdsPerDay.Get();

  const CreativeNewTabPageAdInfo creative_ad = BuildAndSaveCreativeAd();
  const AdEventInfo served_ad_event = BuildAdEvent(
      creative_ad, AdType::kNewTabPageAd, ConfirmationType::kServed, Now());
  FireAdEvents(served_ad_event, ads_per_day - 1);
  const AdEventInfo viewed_ad_event = BuildAdEvent(
      creative_ad, AdType::kNewTabPageAd, ConfirmationType::kViewed, Now());
  FireAdEvents(viewed_ad_event, ads_per_day - 1);

  AdvanceClockBy(base::Days(1) - base::Milliseconds(1));

  const std::string placement_id =
      base::GUID::GenerateRandomV4().AsLowercaseString();

  event_handler_.FireEvent(placement_id, creative_ad.creative_instance_id,
                           mojom::NewTabPageAdEventType::kServed);

  // Act
  event_handler_.FireEvent(placement_id, creative_ad.creative_instance_id,
                           mojom::NewTabPageAdEventType::kViewed);

  // Assert
  EXPECT_EQ(ads_per_day,
            GetAdEventCount(AdType::kNewTabPageAd, ConfirmationType::kServed));
  EXPECT_EQ(ads_per_day,
            GetAdEventCount(AdType::kNewTabPageAd, ConfirmationType::kViewed));
}

TEST_F(BraveAdsNewTabPageAdEventHandlerIfAdsDisabledTest,
       DoNotFireEventIfExceededAdsPerDayCap) {
  // Arrange
  ForcePermissionRulesForTesting();

  const size_t ads_per_day = kMaximumNewTabPageAdsPerDay.Get();

  const CreativeNewTabPageAdInfo creative_ad = BuildAndSaveCreativeAd();
  const AdEventInfo served_ad_event = BuildAdEvent(
      creative_ad, AdType::kNewTabPageAd, ConfirmationType::kServed, Now());
  FireAdEvents(served_ad_event, ads_per_day);
  const AdEventInfo viewed_ad_event = BuildAdEvent(
      creative_ad, AdType::kNewTabPageAd, ConfirmationType::kViewed, Now());
  FireAdEvents(viewed_ad_event, ads_per_day);

  AdvanceClockBy(base::Days(1) - base::Milliseconds(1));

  const std::string placement_id =
      base::GUID::GenerateRandomV4().AsLowercaseString();

  // Act
  event_handler_.FireEvent(placement_id, creative_ad.creative_instance_id,
                           mojom::NewTabPageAdEventType::kViewed);

  // Assert
  EXPECT_EQ(ads_per_day,
            GetAdEventCount(AdType::kNewTabPageAd, ConfirmationType::kServed));
  EXPECT_EQ(ads_per_day,
            GetAdEventCount(AdType::kNewTabPageAd, ConfirmationType::kViewed));
}

}  // namespace brave_ads
