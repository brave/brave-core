/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/test/mock_callback.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_test_constants.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_url_request_builder_util.h"
#include "brave/components/brave_ads/core/internal/common/test/mock_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/permission_rules_test_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings_test_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/ads.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsPromotedContentAdIntegrationTest : public test::TestBase {
 protected:
  void SetUp() override {
    test::TestBase::SetUp(/*is_integration_test=*/true);

    test::ForcePermissionRules();
  }

  void SetUpMocks() override {
    const test::URLResponseMap url_responses = {
        {BuildCatalogUrlPath(),
         {{net::HTTP_OK,
           /*response_body=*/"/catalog_with_promoted_content_ad.json"}}}};
    test::MockUrlResponses(ads_client_mock_, url_responses);

    EXPECT_CALL(ads_client_mock_, RecordP2AEvents).Times(0);
  }

  void TriggerPromotedContentAdEventAndVerifiyExpectations(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      mojom::PromotedContentAdEventType mojom_ad_event_type,
      bool should_fire_event) {
    base::MockCallback<TriggerAdEventCallback> callback;
    EXPECT_CALL(callback, Run(/*success=*/should_fire_event));
    GetAds().TriggerPromotedContentAdEvent(placement_id, creative_instance_id,
                                           mojom_ad_event_type, callback.Get());
  }
};

TEST_F(BraveAdsPromotedContentAdIntegrationTest, TriggerViewedEvent) {
  // Act & Assert
  TriggerPromotedContentAdEventAndVerifiyExpectations(
      test::kPlacementId, test::kCreativeInstanceId,
      mojom::PromotedContentAdEventType::kViewedImpression,
      /*should_fire_event=*/true);
}

TEST_F(BraveAdsPromotedContentAdIntegrationTest, TriggerClickedEvent) {
  // Arrange
  TriggerPromotedContentAdEventAndVerifiyExpectations(
      test::kPlacementId, test::kCreativeInstanceId,
      mojom::PromotedContentAdEventType::kViewedImpression,
      /*should_fire_event=*/true);

  // Act & Assert
  TriggerPromotedContentAdEventAndVerifiyExpectations(
      test::kPlacementId, test::kCreativeInstanceId,
      mojom::PromotedContentAdEventType::kClicked,
      /*should_fire_event=*/true);
}

TEST_F(BraveAdsPromotedContentAdIntegrationTest,
       DoNotTriggerEventForInvalidCreativeInstanceId) {
  // Act & Assert
  TriggerPromotedContentAdEventAndVerifiyExpectations(
      test::kPlacementId, test::kInvalidCreativeInstanceId,
      mojom::PromotedContentAdEventType::kViewedImpression,
      /*should_fire_event=*/false);
}

TEST_F(BraveAdsPromotedContentAdIntegrationTest,
       DoNotTriggerEventIfUserHasNotOptedInToBraveNewsAds) {
  // Arrange
  test::OptOutOfBraveNewsAds();

  // Act & Assert
  TriggerPromotedContentAdEventAndVerifiyExpectations(
      test::kPlacementId, test::kInvalidCreativeInstanceId,
      mojom::PromotedContentAdEventType::kViewedImpression,
      /*should_fire_event=*/false);
}

}  // namespace brave_ads
