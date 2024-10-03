/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <optional>

#include "base/test/mock_callback.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_test_constants.h"
#include "brave/components/brave_ads/core/internal/analytics/p2a/opportunities/p2a_opportunity_util.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_url_request_builder_util.h"
#include "brave/components/brave_ads/core/internal/common/test/mock_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/permission_rules_test_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings_test_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/ad_units/new_tab_page_ad/new_tab_page_ad_info.h"
#include "brave/components/brave_ads/core/public/ads.h"
#include "brave/components/brave_ads/core/public/ads_feature.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsNewTabPageAdIntegrationTest : public test::TestBase {
 protected:
  void SetUp() override { test::TestBase::SetUp(/*is_integration_test=*/true); }

  void SetUpMocks() override {
    const test::URLResponseMap url_responses = {
        {BuildCatalogUrlPath(),
         {{net::HTTP_OK,
           /*response_body=*/"/catalog_with_new_tab_page_ad.json"}}}};
    test::MockUrlResponses(ads_client_mock_, url_responses);
  }

  void TriggerNewTabPageAdEventAndVerifiyExpectations(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      const mojom::NewTabPageAdEventType mojom_ad_event_type,
      const bool should_fire_event) {
    base::MockCallback<TriggerAdEventCallback> callback;
    EXPECT_CALL(callback, Run(/*success=*/should_fire_event));
    GetAds().TriggerNewTabPageAdEvent(placement_id, creative_instance_id,
                                      mojom_ad_event_type, callback.Get());
  }
};

TEST_F(BraveAdsNewTabPageAdIntegrationTest, ServeAd) {
  // Arrange
  const base::test::ScopedFeatureList scoped_feature_list(
      kShouldAlwaysTriggerBraveNewTabPageAdEventsFeature);

  test::ForcePermissionRules();

  // Act & Assert
  EXPECT_CALL(ads_client_mock_,
              RecordP2AEvents(BuildP2AAdOpportunityEvents(
                  mojom::AdType::kNewTabPageAd, /*segments=*/{})));

  base::MockCallback<MaybeServeNewTabPageAdCallback> callback;
  EXPECT_CALL(callback, Run(/*statement=*/::testing::Ne(std::nullopt)));
  GetAds().MaybeServeNewTabPageAd(callback.Get());
}

TEST_F(BraveAdsNewTabPageAdIntegrationTest,
       DoNotServeAdIfPermissionRulesAreDenied) {
  // Arrange
  const base::test::ScopedFeatureList scoped_feature_list(
      kShouldAlwaysTriggerBraveNewTabPageAdEventsFeature);

  // Act & Assert
  EXPECT_CALL(ads_client_mock_, RecordP2AEvents).Times(0);

  base::MockCallback<MaybeServeNewTabPageAdCallback> callback;
  EXPECT_CALL(callback, Run(/*ad=*/::testing::Eq(std::nullopt)));
  GetAds().MaybeServeNewTabPageAd(callback.Get());
}

TEST_F(BraveAdsNewTabPageAdIntegrationTest,
       DoNotServeAdIfUserHasNotOptedInToNewTabPageAds) {
  // Arrange
  const base::test::ScopedFeatureList scoped_feature_list(
      kShouldAlwaysTriggerBraveNewTabPageAdEventsFeature);

  test::ForcePermissionRules();

  test::OptOutOfNewTabPageAds();

  // Act & Assert
  EXPECT_CALL(ads_client_mock_, RecordP2AEvents).Times(0);

  base::MockCallback<MaybeServeNewTabPageAdCallback> callback;
  EXPECT_CALL(callback, Run(/*ad=*/::testing::Eq(std::nullopt)));
  GetAds().MaybeServeNewTabPageAd(callback.Get());
}

TEST_F(
    BraveAdsNewTabPageAdIntegrationTest,
    DoNotServeAdIfShouldNotAlwaysTriggerEventsAndUserHasNotJoinedBraveRewards) {
  // Arrange
  test::ForcePermissionRules();

  test::DisableBraveRewards();

  // Act & Assert
  EXPECT_CALL(ads_client_mock_, RecordP2AEvents).Times(0);

  base::MockCallback<MaybeServeNewTabPageAdCallback> callback;
  EXPECT_CALL(callback, Run(/*statement=*/::testing::Eq(std::nullopt)));
  GetAds().MaybeServeNewTabPageAd(callback.Get());
}

TEST_F(BraveAdsNewTabPageAdIntegrationTest, TriggerViewedEvent) {
  // Arrange
  const base::test::ScopedFeatureList scoped_feature_list(
      kShouldAlwaysTriggerBraveNewTabPageAdEventsFeature);

  test::ForcePermissionRules();

  base::MockCallback<MaybeServeNewTabPageAdCallback> callback;
  EXPECT_CALL(callback, Run)
      .WillOnce([&](const std::optional<NewTabPageAdInfo>& ad) {
        ASSERT_TRUE(ad);
        ASSERT_TRUE(ad->IsValid());

        // Act & Assert
        TriggerNewTabPageAdEventAndVerifiyExpectations(
            ad->placement_id, ad->creative_instance_id,
            mojom::NewTabPageAdEventType::kViewedImpression,
            /*should_fire_event=*/true);
      });

  GetAds().MaybeServeNewTabPageAd(callback.Get());
}

TEST_F(BraveAdsNewTabPageAdIntegrationTest,
       TriggerViewedEventForNonRewardsUser) {
  // Arrange
  const base::test::ScopedFeatureList scoped_feature_list(
      kShouldAlwaysTriggerBraveNewTabPageAdEventsFeature);

  test::DisableBraveRewards();

  // Act & Assert
  TriggerNewTabPageAdEventAndVerifiyExpectations(
      test::kPlacementId, test::kCreativeInstanceId,
      mojom::NewTabPageAdEventType::kViewedImpression,
      /*should_fire_event=*/true);
}

TEST_F(
    BraveAdsNewTabPageAdIntegrationTest,
    DoNotTriggerViewedEventIfShouldNotAlwaysTriggerAdEventsForNonRewardsUser) {
  // Arrange
  test::DisableBraveRewards();

  // Act & Assert
  TriggerNewTabPageAdEventAndVerifiyExpectations(
      test::kPlacementId, test::kCreativeInstanceId,
      mojom::NewTabPageAdEventType::kViewedImpression,
      /*should_fire_event=*/false);
}

TEST_F(BraveAdsNewTabPageAdIntegrationTest, TriggerClickedEvent) {
  // Arrange
  const base::test::ScopedFeatureList scoped_feature_list(
      kShouldAlwaysTriggerBraveNewTabPageAdEventsFeature);

  test::ForcePermissionRules();

  base::MockCallback<MaybeServeNewTabPageAdCallback> callback;
  EXPECT_CALL(callback, Run)
      .WillOnce([&](const std::optional<NewTabPageAdInfo>& ad) {
        ASSERT_TRUE(ad);
        ASSERT_TRUE(ad->IsValid());

        TriggerNewTabPageAdEventAndVerifiyExpectations(
            ad->placement_id, ad->creative_instance_id,
            mojom::NewTabPageAdEventType::kViewedImpression,
            /*should_fire_event=*/true);

        // Act & Assert
        TriggerNewTabPageAdEventAndVerifiyExpectations(
            ad->placement_id, ad->creative_instance_id,
            mojom::NewTabPageAdEventType::kClicked,
            /*should_fire_event=*/true);
      });

  GetAds().MaybeServeNewTabPageAd(callback.Get());
}

TEST_F(BraveAdsNewTabPageAdIntegrationTest,
       TriggerClickedEventForNonRewardsUser) {
  // Arrange
  const base::test::ScopedFeatureList scoped_feature_list(
      kShouldAlwaysTriggerBraveNewTabPageAdEventsFeature);

  test::DisableBraveRewards();

  TriggerNewTabPageAdEventAndVerifiyExpectations(
      test::kPlacementId, test::kCreativeInstanceId,
      mojom::NewTabPageAdEventType::kViewedImpression,
      /*should_fire_event=*/true);

  // Act & Assert
  TriggerNewTabPageAdEventAndVerifiyExpectations(
      test::kPlacementId, test::kCreativeInstanceId,
      mojom::NewTabPageAdEventType::kClicked,
      /*should_fire_event=*/true);
}

TEST_F(
    BraveAdsNewTabPageAdIntegrationTest,
    DoNotTriggerClickedEventIfShouldNotAlwaysTriggerAdEventsForNonRewardsUser) {
  // Arrange
  test::DisableBraveRewards();

  TriggerNewTabPageAdEventAndVerifiyExpectations(
      test::kPlacementId, test::kCreativeInstanceId,
      mojom::NewTabPageAdEventType::kServedImpression,
      /*should_fire_event=*/false);
  TriggerNewTabPageAdEventAndVerifiyExpectations(
      test::kPlacementId, test::kCreativeInstanceId,
      mojom::NewTabPageAdEventType::kViewedImpression,
      /*should_fire_event=*/false);

  // Act & Assert
  TriggerNewTabPageAdEventAndVerifiyExpectations(
      test::kPlacementId, test::kCreativeInstanceId,
      mojom::NewTabPageAdEventType::kClicked,
      /*should_fire_event=*/false);
}

TEST_F(BraveAdsNewTabPageAdIntegrationTest,
       DoNotTriggerEventForInvalidCreativeInstanceId) {
  // Arrange
  const base::test::ScopedFeatureList scoped_feature_list(
      kShouldAlwaysTriggerBraveNewTabPageAdEventsFeature);

  test::ForcePermissionRules();

  base::MockCallback<MaybeServeNewTabPageAdCallback> callback;
  EXPECT_CALL(callback, Run)
      .WillOnce([&](const std::optional<NewTabPageAdInfo>& ad) {
        ASSERT_TRUE(ad);
        ASSERT_TRUE(ad->IsValid());

        // Act & Assert
        TriggerNewTabPageAdEventAndVerifiyExpectations(
            ad->placement_id, test::kInvalidCreativeInstanceId,
            mojom::NewTabPageAdEventType::kViewedImpression,
            /*should_fire_event=*/false);
      });

  GetAds().MaybeServeNewTabPageAd(callback.Get());
}

}  // namespace brave_ads
