/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/ad_type.h"
#include "brave/components/brave_ads/core/confirmation_type.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions_unittest_util.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/ad_event_unittest_util.h"
#include "brave/components/brave_ads/core/internal/ads/ad_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/ads/serving/permission_rules/permission_rules_unittest_util.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_url_request_builder_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_mock_util.h"
#include "brave/components/brave_ads/core/internal/history/history_unittest_util.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsPromotedContentAdIntegrationTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUpForTesting(/*is_integration_test*/ true);

    ForcePermissionRulesForTesting();
  }

  void SetUpMocks() override {
    const URLResponseMap url_responses = {
        {BuildCatalogUrlPath(),
         {{net::HTTP_OK,
           /*response_body*/ "/catalog_with_promoted_content_ad.json"}}}};
    MockUrlResponses(ads_client_mock_, url_responses);
  }
};

TEST_F(BraveAdsPromotedContentAdIntegrationTest, TriggerViewedEvent) {
  // Arrange
  GetAds().TriggerPromotedContentAdEvent(
      kPlacementId, kCreativeInstanceId,
      mojom::PromotedContentAdEventType::kServed);

  // Act
  GetAds().TriggerPromotedContentAdEvent(
      kPlacementId, kCreativeInstanceId,
      mojom::PromotedContentAdEventType::kViewed);

  // Assert
  EXPECT_EQ(1U, GetAdEventCount(AdType::kPromotedContentAd,
                                ConfirmationType::kServed));
  EXPECT_EQ(1U, GetAdEventCount(AdType::kPromotedContentAd,
                                ConfirmationType::kViewed));
  EXPECT_EQ(1U, GetHistoryItemCount());
  EXPECT_EQ(1U, GetTransactionCount());
}

TEST_F(BraveAdsPromotedContentAdIntegrationTest, TriggerClickedEvent) {
  // Arrange
  GetAds().TriggerPromotedContentAdEvent(
      kPlacementId, kCreativeInstanceId,
      mojom::PromotedContentAdEventType::kServed);
  GetAds().TriggerPromotedContentAdEvent(
      kPlacementId, kCreativeInstanceId,
      mojom::PromotedContentAdEventType::kViewed);

  // Act
  GetAds().TriggerPromotedContentAdEvent(
      kPlacementId, kCreativeInstanceId,
      mojom::PromotedContentAdEventType::kClicked);

  // Assert
  EXPECT_EQ(1U, GetAdEventCount(AdType::kPromotedContentAd,
                                ConfirmationType::kServed));
  EXPECT_EQ(1U, GetAdEventCount(AdType::kPromotedContentAd,
                                ConfirmationType::kViewed));
  EXPECT_EQ(1U, GetAdEventCount(AdType::kPromotedContentAd,
                                ConfirmationType::kClicked));
  EXPECT_EQ(2U, GetHistoryItemCount());
  EXPECT_EQ(2U, GetTransactionCount());
}

}  // namespace brave_ads
