/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/test/mock_callback.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/core/internal/analytics/p2a/opportunities/p2a_opportunity_util.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_url_request_builder_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_mock_util.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/permission_rules_unittest_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings_unittest_util.h"
#include "brave/components/brave_ads/core/internal/units/ad_unittest_constants.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-shared.h"
#include "brave/components/brave_ads/core/public/ads_feature.h"
#include "brave/components/brave_ads/core/public/units/ad_type.h"
#include "brave/components/brave_ads/core/public/units/new_tab_page_ad/new_tab_page_ad_info.h"
#include "net/http/http_status_code.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsNewTabPageAdIntegrationTest : public UnitTestBase {
 protected:
  void SetUp() override { UnitTestBase::SetUp(/*is_integration_test=*/true); }

  void SetUpMocks() override {
    const URLResponseMap url_responses = {
        {BuildCatalogUrlPath(),
         {{net::HTTP_OK,
           /*response_body=*/"/catalog_with_new_tab_page_ad.json"}}}};
    MockUrlResponses(ads_client_mock_, url_responses);
  }

  void TriggerNewTabPageAdEvent(const std::string& placement_id,
                                const std::string& creative_instance_id,
                                const mojom::NewTabPageAdEventType& event_type,
                                const bool should_fire_event) {
    base::MockCallback<TriggerAdEventCallback> callback;
    EXPECT_CALL(callback, Run(/*success=*/should_fire_event));
    GetAds().TriggerNewTabPageAdEvent(placement_id, creative_instance_id,
                                      event_type, callback.Get());
  }

  void TriggerNewTabPageAdEvents(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      const std::vector<mojom::NewTabPageAdEventType>& event_types,
      const bool should_fire_event) {
    for (const auto& event_type : event_types) {
      TriggerNewTabPageAdEvent(placement_id, creative_instance_id, event_type,
                               should_fire_event);
    }
  }
};

TEST_F(BraveAdsNewTabPageAdIntegrationTest, ServeAd) {
  // Arrange
  const base::test::ScopedFeatureList scoped_feature_list(
      kShouldAlwaysTriggerBraveNewTabPageAdEventsFeature);

  test::ForcePermissionRules();

  // Act & Assert
  EXPECT_CALL(ads_client_mock_, RecordP2AEvents(BuildP2AAdOpportunityEvents(
                                    AdType::kNewTabPageAd, /*segments=*/{})));

  base::MockCallback<MaybeServeNewTabPageAdCallback> callback;
  EXPECT_CALL(callback, Run(::testing::Ne(absl::nullopt)));
  GetAds().MaybeServeNewTabPageAd(callback.Get());
}

TEST_F(BraveAdsNewTabPageAdIntegrationTest, DoNotServe) {
  // Arrange
  const base::test::ScopedFeatureList scoped_feature_list(
      kShouldAlwaysTriggerBraveNewTabPageAdEventsFeature);

  // Act & Assert
  EXPECT_CALL(ads_client_mock_, RecordP2AEvents).Times(0);

  base::MockCallback<MaybeServeNewTabPageAdCallback> callback;
  EXPECT_CALL(callback, Run(/*ad=*/::testing::Eq(absl::nullopt)));
  GetAds().MaybeServeNewTabPageAd(callback.Get());
}

TEST_F(BraveAdsNewTabPageAdIntegrationTest, TriggerViewedEvent) {
  // Arrange
  const base::test::ScopedFeatureList scoped_feature_list(
      kShouldAlwaysTriggerBraveNewTabPageAdEventsFeature);

  test::ForcePermissionRules();

  base::MockCallback<MaybeServeNewTabPageAdCallback> callback;
  EXPECT_CALL(callback, Run)
      .WillOnce([=](const absl::optional<NewTabPageAdInfo>& ad) {
        ASSERT_TRUE(ad);
        ASSERT_TRUE(ad->IsValid());

        // Act & Assert
        TriggerNewTabPageAdEvent(ad->placement_id, ad->creative_instance_id,
                                 mojom::NewTabPageAdEventType::kViewed,
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
  TriggerNewTabPageAdEvent(kPlacementId, kCreativeInstanceId,
                           mojom::NewTabPageAdEventType::kViewed,
                           /*should_fire_event=*/true);
}

TEST_F(
    BraveAdsNewTabPageAdIntegrationTest,
    DoNotTriggerViewedEventIfShouldNotAlwaysTriggerAdEventsAndRewardsAreDisabled) {
  // Arrange
  test::DisableBraveRewards();

  // Act & Assert
  TriggerNewTabPageAdEvent(kPlacementId, kCreativeInstanceId,
                           mojom::NewTabPageAdEventType::kViewed,
                           /*should_fire_event=*/false);
}

TEST_F(BraveAdsNewTabPageAdIntegrationTest, TriggerClickedEvent) {
  // Arrange
  const base::test::ScopedFeatureList scoped_feature_list(
      kShouldAlwaysTriggerBraveNewTabPageAdEventsFeature);

  test::ForcePermissionRules();

  base::MockCallback<MaybeServeNewTabPageAdCallback> callback;
  EXPECT_CALL(callback, Run)
      .WillOnce([=](const absl::optional<NewTabPageAdInfo>& ad) {
        ASSERT_TRUE(ad);
        ASSERT_TRUE(ad->IsValid());

        TriggerNewTabPageAdEvent(ad->placement_id, ad->creative_instance_id,
                                 mojom::NewTabPageAdEventType::kViewed,
                                 /*should_fire_event=*/true);

        // Act & Assert
        TriggerNewTabPageAdEvent(ad->placement_id, ad->creative_instance_id,
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

  TriggerNewTabPageAdEvent(kPlacementId, kCreativeInstanceId,
                           mojom::NewTabPageAdEventType::kViewed,
                           /*should_fire_event=*/true);

  // Act & Assert
  TriggerNewTabPageAdEvent(kPlacementId, kCreativeInstanceId,
                           mojom::NewTabPageAdEventType::kClicked,
                           /*should_fire_event=*/true);
}

TEST_F(
    BraveAdsNewTabPageAdIntegrationTest,
    DoNotTriggerClickedEventIfShouldNotAlwaysTriggerAdEventsAndBraveRewardsAreDisabled) {
  // Arrange
  test::DisableBraveRewards();

  TriggerNewTabPageAdEvent(kPlacementId, kCreativeInstanceId,
                           mojom::NewTabPageAdEventType::kServed,
                           /*should_fire_event=*/false);
  TriggerNewTabPageAdEvent(kPlacementId, kCreativeInstanceId,
                           mojom::NewTabPageAdEventType::kViewed,
                           /*should_fire_event=*/false);

  // Act & Assert
  TriggerNewTabPageAdEvent(kPlacementId, kCreativeInstanceId,
                           mojom::NewTabPageAdEventType::kClicked,
                           /*should_fire_event=*/false);
}

}  // namespace brave_ads
