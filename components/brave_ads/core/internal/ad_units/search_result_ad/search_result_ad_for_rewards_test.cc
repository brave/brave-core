/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/test/test_future.h"
#include "brave/components/brave_ads/core/internal/ad_units/search_result_ad/search_result_ad_handler.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/creatives/search_result_ads/test/creative_search_result_ad_test_util.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/test/permission_rules_test_util.h"
#include "brave/components/brave_ads/core/public/ads.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsSearchResultAdForRewardsIntegrationTest : public test::TestBase {
 protected:
  void SetUp() override {
    test::TestBase::SetUp(/*is_integration_test=*/true);

    test::ForcePermissionRules();
  }

  void TriggerSearchResultAdEventAndVerifyExpectations(
      mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad,
      mojom::SearchResultAdEventType mojom_ad_event_type,
      bool should_fire_event) {
    base::test::TestFuture<bool> test_future;
    GetAds().TriggerSearchResultAdEvent(std::move(mojom_creative_ad),
                                        mojom_ad_event_type,
                                        test_future.GetCallback());
    EXPECT_EQ(should_fire_event, test_future.Get());
  }
};

TEST_F(BraveAdsSearchResultAdForRewardsIntegrationTest, TriggerViewedEvents) {
  // Act & Assert
  TriggerSearchResultAdEventAndVerifyExpectations(
      test::BuildCreativeSearchResultAdWithConversion(
          /*use_random_uuids=*/true),
      mojom::SearchResultAdEventType::kViewedImpression,
      /*should_fire_event=*/true);

  TriggerSearchResultAdEventAndVerifyExpectations(
      test::BuildCreativeSearchResultAd(/*use_random_uuids=*/true),
      mojom::SearchResultAdEventType::kViewedImpression,
      /*should_fire_event=*/true);
}

TEST_F(BraveAdsSearchResultAdForRewardsIntegrationTest,
       TriggerDeferredViewedEvents) {
  // Arrange
  SearchResultAdHandler::DeferTriggeringAdViewedEventForTesting();

  TriggerSearchResultAdEventAndVerifyExpectations(
      // This viewed impression ad event will be deferred.
      test::BuildCreativeSearchResultAdWithConversion(
          /*use_random_uuids=*/true),
      mojom::SearchResultAdEventType::kViewedImpression,
      /*should_fire_event=*/true);

  // Act & Assert
  TriggerSearchResultAdEventAndVerifyExpectations(
      // This viewed impression ad event will be deferred as the previous viewed
      // impression ad event has not fired.
      test::BuildCreativeSearchResultAd(/*use_random_uuids=*/true),
      mojom::SearchResultAdEventType::kViewedImpression,
      /*should_fire_event=*/true);

  SearchResultAdHandler::TriggerDeferredAdViewedEventForTesting();
}

TEST_F(BraveAdsSearchResultAdForRewardsIntegrationTest, TriggerClickedEvent) {
  // Arrange
  const mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad =
      test::BuildCreativeSearchResultAd(/*use_random_uuids=*/true);

  TriggerSearchResultAdEventAndVerifyExpectations(
      mojom_creative_ad.Clone(),
      mojom::SearchResultAdEventType::kViewedImpression,
      /*should_fire_event=*/true);

  // Act & Assert
  TriggerSearchResultAdEventAndVerifyExpectations(
      mojom_creative_ad.Clone(), mojom::SearchResultAdEventType::kClicked,
      /*should_fire_event=*/true);
}

}  // namespace brave_ads
