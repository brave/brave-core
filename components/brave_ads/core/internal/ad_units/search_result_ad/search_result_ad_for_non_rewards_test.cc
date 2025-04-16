/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/run_loop.h"
#include "base/test/gmock_callback_support.h"
#include "base/test/mock_callback.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/core/internal/ad_units/search_result_ad/search_result_ad_handler.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/creatives/search_result_ads/creative_search_result_ad_test_util.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/permission_rules_test_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings_test_util.h"
#include "brave/components/brave_ads/core/public/ads.h"
#include "brave/components/brave_ads/core/public/ads_feature.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsSearchResultAdForNonRewardsIntegrationTest
    : public test::TestBase {
 protected:
  void SetUp() override {
    test::TestBase::SetUp(/*is_integration_test=*/true);

    test::ForcePermissionRules();

    test::DisableBraveRewards();
  }

  void SetUpMocks() override {
    EXPECT_CALL(ads_client_mock_, RecordP2AEvents).Times(0);
  }

  void TriggerSearchResultAdEventAndVerifyExpectations(
      mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad,
      mojom::SearchResultAdEventType mojom_ad_event_type,
      bool should_fire_event) {
    base::RunLoop run_loop;
    base::MockCallback<TriggerAdEventCallback> callback;
    EXPECT_CALL(callback, Run(/*success=*/should_fire_event))
        .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
    GetAds().TriggerSearchResultAdEvent(std::move(mojom_creative_ad),
                                        mojom_ad_event_type, callback.Get());
    run_loop.Run();
  }
};

TEST_F(BraveAdsSearchResultAdForNonRewardsIntegrationTest,
       DoNotTriggerViewedEvent) {
  // Arrange
  const base::test::ScopedFeatureList scoped_feature_list(
      {kShouldAlwaysTriggerBraveSearchResultAdEventsFeature});

  // Act & Assert
  TriggerSearchResultAdEventAndVerifyExpectations(
      test::BuildCreativeSearchResultAdWithConversion(
          /*should_generate_random_uuids=*/true),
      mojom::SearchResultAdEventType::kViewedImpression,
      /*should_fire_event=*/false);
}

TEST_F(BraveAdsSearchResultAdForNonRewardsIntegrationTest,
       DoNotTriggerDeferredViewedEvents) {
  // Arrange
  const base::test::ScopedFeatureList scoped_feature_list(
      {kShouldAlwaysTriggerBraveSearchResultAdEventsFeature});

  SearchResultAdHandler::DeferTriggeringAdViewedEventForTesting();

  TriggerSearchResultAdEventAndVerifyExpectations(
      // This viewed impression ad event will be deferred.
      test::BuildCreativeSearchResultAdWithConversion(
          /*should_generate_random_uuids=*/true),
      mojom::SearchResultAdEventType::kViewedImpression,
      /*should_fire_event=*/false);

  // Act & Assert
  TriggerSearchResultAdEventAndVerifyExpectations(
      // This viewed impression ad event will be deferred as the previous viewed
      // impression ad event has not fired.
      test::BuildCreativeSearchResultAdWithConversion(
          /*should_generate_random_uuids=*/true),
      mojom::SearchResultAdEventType::kViewedImpression,
      /*should_fire_event=*/false);

  SearchResultAdHandler::TriggerDeferredAdViewedEventForTesting();
}

TEST_F(BraveAdsSearchResultAdForNonRewardsIntegrationTest,
       TriggerClickedEvent) {
  // Arrange
  const base::test::ScopedFeatureList scoped_feature_list(
      {kShouldAlwaysTriggerBraveSearchResultAdEventsFeature});

  const mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad =
      test::BuildCreativeSearchResultAdWithConversion(
          /*should_generate_random_uuids=*/true);

  // Act & Assert
  TriggerSearchResultAdEventAndVerifyExpectations(
      mojom_creative_ad.Clone(), mojom::SearchResultAdEventType::kClicked,
      /*should_fire_event=*/true);
}

}  // namespace brave_ads
