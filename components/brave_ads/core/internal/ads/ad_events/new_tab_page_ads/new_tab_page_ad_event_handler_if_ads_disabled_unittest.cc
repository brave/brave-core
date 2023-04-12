/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/ad_events/new_tab_page_ads/new_tab_page_ad_event_handler.h"

#include <memory>

#include "base/guid.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_ads/core/ad_type.h"
#include "brave/components/brave_ads/core/confirmation_type.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/ad_event_info.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/ad_event_unittest_util.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/new_tab_page_ads/new_tab_page_ad_event_handler_observer.h"
#include "brave/components/brave_ads/core/internal/ads/ad_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/ads/new_tab_page_ad_features.h"
#include "brave/components/brave_ads/core/internal/ads/serving/permission_rules/permission_rules_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/new_tab_page_ad_builder.h"
#include "brave/components/brave_ads/core/new_tab_page_ad_info.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace brave_ads::new_tab_page_ads {

namespace {

CreativeNewTabPageAdInfo BuildAndSaveCreativeAd() {
  CreativeNewTabPageAdInfo creative_ad =
      BuildCreativeNewTabPageAd(/*should_use_random_guids*/ true);

  SaveCreativeAds({creative_ad});

  return creative_ad;
}

}  // namespace

class BatAdsNewTabPageAdEventHandlerIfAdsDisabledTest
    : public EventHandlerObserver,
      public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    AdsClientHelper::GetInstance()->SetBooleanPref(prefs::kEnabled, false);

    event_handler_ = std::make_unique<EventHandler>();
    event_handler_->AddObserver(this);
  }

  void TearDown() override {
    event_handler_->RemoveObserver(this);

    UnitTestBase::TearDown();
  }

  void OnNewTabPageAdServed(const NewTabPageAdInfo& ad) override {
    ad_ = ad;
    did_serve_ad_ = true;
  }

  void OnNewTabPageAdViewed(const NewTabPageAdInfo& ad) override {
    ad_ = ad;
    did_view_ad_ = true;
  }

  void OnNewTabPageAdClicked(const NewTabPageAdInfo& ad) override {
    ad_ = ad;
    did_click_ad_ = true;
  }

  void OnNewTabPageAdEventFailed(
      const std::string& /*placement_id*/,
      const std::string& /*creative_instance_id*/,
      const mojom::NewTabPageAdEventType /*event_type*/) override {
    did_fail_to_fire_event_ = true;
  }

  std::unique_ptr<EventHandler> event_handler_;

  NewTabPageAdInfo ad_;
  bool did_serve_ad_ = false;
  bool did_view_ad_ = false;
  bool did_click_ad_ = false;
  bool did_fail_to_fire_event_ = false;
};

TEST_F(BatAdsNewTabPageAdEventHandlerIfAdsDisabledTest, FireViewedEvent) {
  // Arrange
  ForcePermissionRulesForTesting();

  const CreativeNewTabPageAdInfo creative_ad = BuildAndSaveCreativeAd();

  event_handler_->FireEvent(kPlacementId, creative_ad.creative_instance_id,
                            mojom::NewTabPageAdEventType::kServed);

  // Act
  event_handler_->FireEvent(kPlacementId, creative_ad.creative_instance_id,
                            mojom::NewTabPageAdEventType::kViewed);

  // Assert
  EXPECT_TRUE(did_serve_ad_);
  EXPECT_TRUE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_FALSE(did_fail_to_fire_event_);
  const NewTabPageAdInfo expected_ad =
      BuildNewTabPageAd(creative_ad, kPlacementId);
  EXPECT_EQ(expected_ad, ad_);
  EXPECT_EQ(1,
            GetAdEventCount(AdType::kNewTabPageAd, ConfirmationType::kServed));
  EXPECT_EQ(1,
            GetAdEventCount(AdType::kNewTabPageAd, ConfirmationType::kViewed));
}

TEST_F(BatAdsNewTabPageAdEventHandlerIfAdsDisabledTest, FireClickedEvent) {
  // Arrange
  ForcePermissionRulesForTesting();

  const CreativeNewTabPageAdInfo creative_ad = BuildAndSaveCreativeAd();

  event_handler_->FireEvent(kPlacementId, creative_ad.creative_instance_id,
                            mojom::NewTabPageAdEventType::kServed);

  event_handler_->FireEvent(kPlacementId, creative_ad.creative_instance_id,
                            mojom::NewTabPageAdEventType::kViewed);

  // Act
  event_handler_->FireEvent(kPlacementId, creative_ad.creative_instance_id,
                            mojom::NewTabPageAdEventType::kClicked);

  // Assert
  EXPECT_TRUE(did_serve_ad_);
  EXPECT_TRUE(did_view_ad_);
  EXPECT_TRUE(did_click_ad_);
  EXPECT_FALSE(did_fail_to_fire_event_);
  const NewTabPageAdInfo expected_ad =
      BuildNewTabPageAd(creative_ad, kPlacementId);
  EXPECT_EQ(expected_ad, ad_);
  EXPECT_EQ(1,
            GetAdEventCount(AdType::kNewTabPageAd, ConfirmationType::kServed));
  EXPECT_EQ(1,
            GetAdEventCount(AdType::kNewTabPageAd, ConfirmationType::kViewed));
  EXPECT_EQ(1,
            GetAdEventCount(AdType::kNewTabPageAd, ConfirmationType::kClicked));
}

TEST_F(BatAdsNewTabPageAdEventHandlerIfAdsDisabledTest,
       DoNotFireViewedEventIfAlreadyFired) {
  // Arrange
  ForcePermissionRulesForTesting();

  const CreativeNewTabPageAdInfo creative_ad = BuildAndSaveCreativeAd();

  event_handler_->FireEvent(kPlacementId, creative_ad.creative_instance_id,
                            mojom::NewTabPageAdEventType::kServed);

  // Act
  event_handler_->FireEvent(kPlacementId, creative_ad.creative_instance_id,
                            mojom::NewTabPageAdEventType::kViewed);

  // Assert
  EXPECT_EQ(1,
            GetAdEventCount(AdType::kNewTabPageAd, ConfirmationType::kServed));
  EXPECT_EQ(1,
            GetAdEventCount(AdType::kNewTabPageAd, ConfirmationType::kViewed));
}

TEST_F(BatAdsNewTabPageAdEventHandlerIfAdsDisabledTest,
       DoNotFireEventWithInvalidPlacementId) {
  // Arrange

  // Act
  event_handler_->FireEvent(kInvalidPlacementId, kCreativeInstanceId,
                            mojom::NewTabPageAdEventType::kViewed);

  // Assert
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_FALSE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_TRUE(did_fail_to_fire_event_);
  EXPECT_EQ(0,
            GetAdEventCount(AdType::kNewTabPageAd, ConfirmationType::kServed));
  EXPECT_EQ(0,
            GetAdEventCount(AdType::kNewTabPageAd, ConfirmationType::kViewed));
}

TEST_F(BatAdsNewTabPageAdEventHandlerIfAdsDisabledTest,
       DoNotFireEventWithInvalidCreativeInstanceId) {
  // Arrange

  // Act
  event_handler_->FireEvent(kPlacementId, kInvalidCreativeInstanceId,
                            mojom::NewTabPageAdEventType::kViewed);

  // Assert
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_FALSE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_TRUE(did_fail_to_fire_event_);
  EXPECT_EQ(0,
            GetAdEventCount(AdType::kNewTabPageAd, ConfirmationType::kServed));
  EXPECT_EQ(0,
            GetAdEventCount(AdType::kNewTabPageAd, ConfirmationType::kViewed));
}

TEST_F(BatAdsNewTabPageAdEventHandlerIfAdsDisabledTest,
       DoNotFireEventWhenNotPermitted) {
  // Arrange
  const CreativeNewTabPageAdInfo creative_ad = BuildAndSaveCreativeAd();

  // Act
  event_handler_->FireEvent(kPlacementId, creative_ad.creative_instance_id,
                            mojom::NewTabPageAdEventType::kViewed);

  // Assert
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_FALSE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_TRUE(did_fail_to_fire_event_);
  EXPECT_EQ(0,
            GetAdEventCount(AdType::kNewTabPageAd, ConfirmationType::kServed));
  EXPECT_EQ(0,
            GetAdEventCount(AdType::kNewTabPageAd, ConfirmationType::kViewed));
}

TEST_F(BatAdsNewTabPageAdEventHandlerIfAdsDisabledTest,
       DoNotFireEventForUnknownCreativeInstanceId) {
  // Arrange
  ForcePermissionRulesForTesting();

  // Act
  event_handler_->FireEvent(kPlacementId, kCreativeInstanceId,
                            mojom::NewTabPageAdEventType::kViewed);

  // Assert
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_FALSE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_TRUE(did_fail_to_fire_event_);
  EXPECT_EQ(0,
            GetAdEventCount(AdType::kNewTabPageAd, ConfirmationType::kServed));
  EXPECT_EQ(0,
            GetAdEventCount(AdType::kNewTabPageAd, ConfirmationType::kViewed));
}

TEST_F(BatAdsNewTabPageAdEventHandlerIfAdsDisabledTest,
       FireEventIfNotExceededAdsPerHourCap) {
  // Arrange
  ForcePermissionRulesForTesting();

  const std::string placement_id =
      base::GUID::GenerateRandomV4().AsLowercaseString();

  const int ads_per_hour = features::GetMaximumAdsPerHour();

  const CreativeNewTabPageAdInfo creative_ad = BuildAndSaveCreativeAd();
  const AdEventInfo served_ad_event = BuildAdEvent(
      creative_ad, AdType::kNewTabPageAd, ConfirmationType::kServed, Now());
  FireAdEvents(served_ad_event, ads_per_hour - 1);
  const AdEventInfo viewed_ad_event = BuildAdEvent(
      creative_ad, AdType::kNewTabPageAd, ConfirmationType::kViewed, Now());
  FireAdEvents(viewed_ad_event, ads_per_hour - 1);

  AdvanceClockBy(features::GetMinimumWaitTime());

  event_handler_->FireEvent(placement_id, creative_ad.creative_instance_id,
                            mojom::NewTabPageAdEventType::kServed);

  // Act
  event_handler_->FireEvent(placement_id, creative_ad.creative_instance_id,
                            mojom::NewTabPageAdEventType::kViewed);

  // Assert
  EXPECT_EQ(ads_per_hour,
            GetAdEventCount(AdType::kNewTabPageAd, ConfirmationType::kServed));
  EXPECT_EQ(ads_per_hour,
            GetAdEventCount(AdType::kNewTabPageAd, ConfirmationType::kViewed));
}

TEST_F(BatAdsNewTabPageAdEventHandlerIfAdsDisabledTest,
       DoNotFireEventIfExceededAdsPerHourCap) {
  // Arrange
  ForcePermissionRulesForTesting();

  const int ads_per_hour = features::GetMaximumAdsPerHour();

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
  event_handler_->FireEvent(placement_id, creative_ad.creative_instance_id,
                            mojom::NewTabPageAdEventType::kViewed);

  // Assert
  EXPECT_EQ(ads_per_hour,
            GetAdEventCount(AdType::kNewTabPageAd, ConfirmationType::kServed));
  EXPECT_EQ(ads_per_hour,
            GetAdEventCount(AdType::kNewTabPageAd, ConfirmationType::kViewed));
}

TEST_F(BatAdsNewTabPageAdEventHandlerIfAdsDisabledTest,
       FireEventIfNotExceededAdsPerDayCap) {
  // Arrange
  ForcePermissionRulesForTesting();

  const int ads_per_day = features::GetMaximumAdsPerDay();

  const CreativeNewTabPageAdInfo creative_ad = BuildAndSaveCreativeAd();
  const AdEventInfo served_ad_event = BuildAdEvent(
      creative_ad, AdType::kNewTabPageAd, ConfirmationType::kServed, Now());
  FireAdEvents(served_ad_event, ads_per_day - 1);
  const AdEventInfo viewed_ad_event = BuildAdEvent(
      creative_ad, AdType::kNewTabPageAd, ConfirmationType::kViewed, Now());
  FireAdEvents(viewed_ad_event, ads_per_day - 1);

  AdvanceClockBy(base::Days(1) - base::Seconds(1));

  const std::string placement_id =
      base::GUID::GenerateRandomV4().AsLowercaseString();

  event_handler_->FireEvent(placement_id, creative_ad.creative_instance_id,
                            mojom::NewTabPageAdEventType::kServed);

  // Act
  event_handler_->FireEvent(placement_id, creative_ad.creative_instance_id,
                            mojom::NewTabPageAdEventType::kViewed);

  // Assert
  EXPECT_EQ(ads_per_day,
            GetAdEventCount(AdType::kNewTabPageAd, ConfirmationType::kServed));
  EXPECT_EQ(ads_per_day,
            GetAdEventCount(AdType::kNewTabPageAd, ConfirmationType::kViewed));
}

TEST_F(BatAdsNewTabPageAdEventHandlerIfAdsDisabledTest,
       DoNotFireEventIfExceededAdsPerDayCap) {
  // Arrange
  ForcePermissionRulesForTesting();

  const int ads_per_day = features::GetMaximumAdsPerDay();

  const CreativeNewTabPageAdInfo creative_ad = BuildAndSaveCreativeAd();
  const AdEventInfo served_ad_event = BuildAdEvent(
      creative_ad, AdType::kNewTabPageAd, ConfirmationType::kServed, Now());
  FireAdEvents(served_ad_event, ads_per_day);
  const AdEventInfo viewed_ad_event = BuildAdEvent(
      creative_ad, AdType::kNewTabPageAd, ConfirmationType::kViewed, Now());
  FireAdEvents(viewed_ad_event, ads_per_day);

  AdvanceClockBy(base::Days(1) - base::Seconds(1));

  const std::string placement_id =
      base::GUID::GenerateRandomV4().AsLowercaseString();

  // Act
  event_handler_->FireEvent(placement_id, creative_ad.creative_instance_id,
                            mojom::NewTabPageAdEventType::kViewed);

  // Assert
  EXPECT_EQ(ads_per_day,
            GetAdEventCount(AdType::kNewTabPageAd, ConfirmationType::kServed));
  EXPECT_EQ(ads_per_day,
            GetAdEventCount(AdType::kNewTabPageAd, ConfirmationType::kViewed));
}

}  // namespace brave_ads::new_tab_page_ads
