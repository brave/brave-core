/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/ad_events/new_tab_page_ads/new_tab_page_ad_event_handler.h"

#include <vector>

#include "base/test/mock_callback.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/ad_event_unittest_util.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/new_tab_page_ads/new_tab_page_ad_event_handler_delegate.h"
#include "brave/components/brave_ads/core/internal/ads/ad_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ads_database_util.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/new_tab_page_ad_builder.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-shared.h"
#include "brave/components/brave_ads/core/public/ad_type.h"
#include "brave/components/brave_ads/core/public/ads/new_tab_page_ad_info.h"
#include "brave/components/brave_ads/core/public/confirmation_type.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

NewTabPageAdInfo BuildAndSaveAd() {
  const CreativeNewTabPageAdInfo creative_ad =
      BuildCreativeNewTabPageAdForTesting(/*should_use_random_uuids*/ true);
  database::SaveCreativeNewTabPageAds({creative_ad});
  return BuildNewTabPageAd(creative_ad);
}

}  // namespace

class BraveAdsNewTabPageAdEventHandlerTest
    : public NewTabPageAdEventHandlerDelegate,
      public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

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

  void FireEvent(const std::string& placement_id,
                 const std::string& creative_instance_id,
                 const mojom::NewTabPageAdEventType& event_type,
                 const bool should_fire_event) {
    base::MockCallback<FireNewTabPageAdEventHandlerCallback> callback;
    EXPECT_CALL(callback,
                Run(/*success*/ should_fire_event, placement_id, event_type));

    event_handler_.FireEvent(placement_id, creative_instance_id, event_type,
                             callback.Get());
  }

  void FireEvents(const std::string& placement_id,
                  const std::string& creative_instance_id,
                  const std::vector<mojom::NewTabPageAdEventType>& event_types,
                  const bool should_fire_event) {
    for (const auto& event_type : event_types) {
      FireEvent(placement_id, creative_instance_id, event_type,
                should_fire_event);
    }
  }

  NewTabPageAdEventHandler event_handler_;

  NewTabPageAdInfo ad_;
  bool did_serve_ad_ = false;
  bool did_view_ad_ = false;
  bool did_click_ad_ = false;
  bool did_fail_to_fire_event_ = false;
};

TEST_F(BraveAdsNewTabPageAdEventHandlerTest, FireServedEvent) {
  // Arrange
  const NewTabPageAdInfo ad = BuildAndSaveAd();

  // Act
  FireEvent(ad.placement_id, ad.creative_instance_id,
            mojom::NewTabPageAdEventType::kServed,
            /*should_fire_event*/ true);

  EXPECT_TRUE(did_serve_ad_);
  EXPECT_FALSE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_FALSE(did_fail_to_fire_event_);
  EXPECT_EQ(ad, ad_);
  EXPECT_EQ(1U, GetAdEventCountForTesting(AdType::kNewTabPageAd,
                                          ConfirmationType::kServed));
}

TEST_F(BraveAdsNewTabPageAdEventHandlerTest, FireViewedEvent) {
  // Arrange
  const NewTabPageAdInfo ad = BuildAndSaveAd();

  FireEvent(ad.placement_id, ad.creative_instance_id,
            mojom::NewTabPageAdEventType::kServed,
            /*should_fire_event*/ true);

  // Act
  FireEvent(ad.placement_id, ad.creative_instance_id,
            mojom::NewTabPageAdEventType::kViewed,
            /*should_fire_event*/ true);

  EXPECT_TRUE(did_serve_ad_);
  EXPECT_TRUE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_FALSE(did_fail_to_fire_event_);
  EXPECT_EQ(ad, ad_);
  EXPECT_EQ(1U, GetAdEventCountForTesting(AdType::kNewTabPageAd,
                                          ConfirmationType::kServed));
  EXPECT_EQ(1U, GetAdEventCountForTesting(AdType::kNewTabPageAd,
                                          ConfirmationType::kViewed));
}

TEST_F(BraveAdsNewTabPageAdEventHandlerTest,
       DoNotFireViewedEventIfAdPlacementWasAlreadyViewed) {
  // Arrange
  const NewTabPageAdInfo ad = BuildAndSaveAd();

  FireEvents(ad.placement_id, ad.creative_instance_id,
             {mojom::NewTabPageAdEventType::kServed,
              mojom::NewTabPageAdEventType::kViewed},
             /*should_fire_event*/ true);

  // Act
  FireEvent(ad.placement_id, ad.creative_instance_id,
            mojom::NewTabPageAdEventType::kViewed, /*should_fire_event*/ false);

  // Assert
  EXPECT_EQ(1U, GetAdEventCountForTesting(AdType::kNewTabPageAd,
                                          ConfirmationType::kServed));
  EXPECT_EQ(1U, GetAdEventCountForTesting(AdType::kNewTabPageAd,
                                          ConfirmationType::kViewed));
}

TEST_F(BraveAdsNewTabPageAdEventHandlerTest,
       DoNotFireViewedEventIfAdPlacementWasNotServed) {
  // Arrange
  const NewTabPageAdInfo ad = BuildAndSaveAd();

  // Act
  FireEvent(ad.placement_id, ad.creative_instance_id,
            mojom::NewTabPageAdEventType::kViewed, /*should_fire_event*/ false);

  // Assert
  EXPECT_EQ(0U, GetAdEventCountForTesting(AdType::kNewTabPageAd,
                                          ConfirmationType::kServed));
  EXPECT_EQ(0U, GetAdEventCountForTesting(AdType::kNewTabPageAd,
                                          ConfirmationType::kViewed));
}

TEST_F(BraveAdsNewTabPageAdEventHandlerTest, FireClickedEvent) {
  // Arrange
  const NewTabPageAdInfo ad = BuildAndSaveAd();

  FireEvents(ad.placement_id, ad.creative_instance_id,
             {mojom::NewTabPageAdEventType::kServed,
              mojom::NewTabPageAdEventType::kViewed},
             /*should_fire_event*/ true);

  // Act
  FireEvent(ad.placement_id, ad.creative_instance_id,
            mojom::NewTabPageAdEventType::kClicked, /*should_fire_event*/ true);

  // Assert
  EXPECT_TRUE(did_serve_ad_);
  EXPECT_TRUE(did_view_ad_);
  EXPECT_TRUE(did_click_ad_);
  EXPECT_FALSE(did_fail_to_fire_event_);
  EXPECT_EQ(ad, ad_);
  EXPECT_EQ(1U, GetAdEventCountForTesting(AdType::kNewTabPageAd,
                                          ConfirmationType::kServed));
  EXPECT_EQ(1U, GetAdEventCountForTesting(AdType::kNewTabPageAd,
                                          ConfirmationType::kViewed));
  EXPECT_EQ(1U, GetAdEventCountForTesting(AdType::kNewTabPageAd,
                                          ConfirmationType::kClicked));
}

TEST_F(BraveAdsNewTabPageAdEventHandlerTest,
       DoNotFireClickedEventIfAdPlacementWasAlreadyClicked) {
  // Arrange
  const NewTabPageAdInfo ad = BuildAndSaveAd();

  FireEvents(ad.placement_id, ad.creative_instance_id,
             {mojom::NewTabPageAdEventType::kServed,
              mojom::NewTabPageAdEventType::kViewed,
              mojom::NewTabPageAdEventType::kClicked},
             /*should_fire_event*/ true);

  // Act
  FireEvent(ad.placement_id, ad.creative_instance_id,
            mojom::NewTabPageAdEventType::kClicked,
            /*should_fire_event*/ false);

  // Assert
  EXPECT_EQ(1U, GetAdEventCountForTesting(AdType::kNewTabPageAd,
                                          ConfirmationType::kServed));
  EXPECT_EQ(1U, GetAdEventCountForTesting(AdType::kNewTabPageAd,
                                          ConfirmationType::kViewed));
  EXPECT_EQ(1U, GetAdEventCountForTesting(AdType::kNewTabPageAd,
                                          ConfirmationType::kClicked));
}

TEST_F(BraveAdsNewTabPageAdEventHandlerTest,
       DoNotFireClickedEventIfAdPlacementWasNotServed) {
  // Arrange
  const NewTabPageAdInfo ad = BuildAndSaveAd();

  // Act
  FireEvent(ad.placement_id, ad.creative_instance_id,
            mojom::NewTabPageAdEventType::kViewed,
            /*should_fire_event*/ false);

  // Assert
  EXPECT_EQ(0U, GetAdEventCountForTesting(AdType::kNewTabPageAd,
                                          ConfirmationType::kServed));
  EXPECT_EQ(0U, GetAdEventCountForTesting(AdType::kNewTabPageAd,
                                          ConfirmationType::kViewed));
}

TEST_F(BraveAdsNewTabPageAdEventHandlerTest,
       DoNotFireEventWithInvalidPlacementId) {
  // Arrange

  // Act
  FireEvent(kInvalidPlacementId, kCreativeInstanceId,
            mojom::NewTabPageAdEventType::kServed,
            /*should_fire_event*/ false);

  // Assert
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_FALSE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_TRUE(did_fail_to_fire_event_);
  EXPECT_EQ(0U, GetAdEventCountForTesting(AdType::kNewTabPageAd,
                                          ConfirmationType::kServed));
}

TEST_F(BraveAdsNewTabPageAdEventHandlerTest,
       DoNotFireEventWithInvalidCreativeInstanceId) {
  // Arrange

  // Act
  FireEvent(kPlacementId, kInvalidCreativeInstanceId,
            mojom::NewTabPageAdEventType::kServed,
            /*should_fire_event*/ false);

  // Assert
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_FALSE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_TRUE(did_fail_to_fire_event_);
  EXPECT_EQ(0U, GetAdEventCountForTesting(AdType::kNewTabPageAd,
                                          ConfirmationType::kServed));
}

TEST_F(BraveAdsNewTabPageAdEventHandlerTest,
       DoNotFireEventForMissingCreativeInstanceId) {
  // Arrange
  const NewTabPageAdInfo ad = BuildAndSaveAd();

  // Act
  FireEvent(ad.placement_id, kMissingCreativeInstanceId,
            mojom::NewTabPageAdEventType::kServed,
            /*should_fire_event*/ false);

  // Assert
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_FALSE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_TRUE(did_fail_to_fire_event_);
  EXPECT_EQ(0U, GetAdEventCountForTesting(AdType::kNewTabPageAd,
                                          ConfirmationType::kServed));
}

}  // namespace brave_ads
