/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/functional/bind.h"
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
#include "brave/components/brave_ads/core/new_tab_page_ad_info.h"
#include "net/http/http_status_code.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsNewTabPageAdIntegrationTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUpForTesting(/*is_integration_test*/ true);

    ForcePermissionRulesForTesting();
  }

  void SetUpMocks() override {
    const URLResponseMap url_responses = {
        {BuildCatalogUrlPath(),
         {{net::HTTP_OK,
           /*response_body*/ "/catalog_with_new_tab_page_ad.json"}}}};
    MockUrlResponses(ads_client_mock_, url_responses);
  }
};

TEST_F(BraveAdsNewTabPageAdIntegrationTest, Serve) {
  // Arrange

  // Act
  GetAds().MaybeServeNewTabPageAd(
      base::BindOnce([](const absl::optional<NewTabPageAdInfo>& ad) {
        // Assert
        EXPECT_TRUE(ad);
        EXPECT_TRUE(ad->IsValid());
        EXPECT_EQ(1U, GetAdEventCount(AdType::kNewTabPageAd,
                                      ConfirmationType::kServed));
      }));
}

TEST_F(BraveAdsNewTabPageAdIntegrationTest, TriggerServedEvent) {
  // Arrange

  // Act
  GetAds().TriggerNewTabPageAdEvent(kPlacementId, kCreativeInstanceId,
                                    mojom::NewTabPageAdEventType::kServed);

  // Assert
  EXPECT_EQ(1U,
            GetAdEventCount(AdType::kNewTabPageAd, ConfirmationType::kServed));
  EXPECT_EQ(0U, GetHistoryItemCount());
  EXPECT_EQ(0U, GetTransactionCount());
}

TEST_F(BraveAdsNewTabPageAdIntegrationTest, TriggerViewedEvent) {
  // Arrange
  GetAds().TriggerNewTabPageAdEvent(kPlacementId, kCreativeInstanceId,
                                    mojom::NewTabPageAdEventType::kServed);

  // Act
  GetAds().TriggerNewTabPageAdEvent(kPlacementId, kCreativeInstanceId,
                                    mojom::NewTabPageAdEventType::kViewed);

  // Assert
  EXPECT_EQ(1U,
            GetAdEventCount(AdType::kNewTabPageAd, ConfirmationType::kServed));
  EXPECT_EQ(1U,
            GetAdEventCount(AdType::kNewTabPageAd, ConfirmationType::kViewed));
  EXPECT_EQ(1U, GetHistoryItemCount());
  EXPECT_EQ(1U, GetTransactionCount());
}

TEST_F(BraveAdsNewTabPageAdIntegrationTest, TriggerClickedEvent) {
  // Arrange
  GetAds().TriggerNewTabPageAdEvent(kPlacementId, kCreativeInstanceId,
                                    mojom::NewTabPageAdEventType::kServed);
  GetAds().TriggerNewTabPageAdEvent(kPlacementId, kCreativeInstanceId,
                                    mojom::NewTabPageAdEventType::kViewed);

  // Act
  GetAds().TriggerNewTabPageAdEvent(kPlacementId, kCreativeInstanceId,
                                    mojom::NewTabPageAdEventType::kClicked);

  // Assert
  EXPECT_EQ(1U,
            GetAdEventCount(AdType::kNewTabPageAd, ConfirmationType::kServed));
  EXPECT_EQ(1U,
            GetAdEventCount(AdType::kNewTabPageAd, ConfirmationType::kViewed));
  EXPECT_EQ(1U,
            GetAdEventCount(AdType::kNewTabPageAd, ConfirmationType::kClicked));
  EXPECT_EQ(2U, GetHistoryItemCount());
  EXPECT_EQ(2U, GetTransactionCount());
}

}  // namespace brave_ads
