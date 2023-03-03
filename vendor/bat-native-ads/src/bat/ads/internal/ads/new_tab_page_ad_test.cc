/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "absl/types/optional.h"
#include "base/functional/bind.h"
#include "bat/ads/ad_type.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/account/transactions/transactions_unittest_util.h"
#include "bat/ads/internal/ads/ad_events/ad_event_unittest_util.h"
#include "bat/ads/internal/ads/serving/permission_rules/permission_rules_unittest_util.h"
#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/common/unittest/unittest_mock_util.h"
#include "bat/ads/internal/history/history_unittest_util.h"
#include "bat/ads/new_tab_page_ad_info.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {

constexpr char kPlacementId[] = "f0948316-df6f-4e31-814d-d0b5f2a1f28c";
constexpr char kCreativeInstanceIdId[] = "7ff400b9-7f8a-46a8-89f1-cb386612edcf";

}  // namespace

class BatAdsNewTabPageAdIntegrationTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUpForTesting(/*is_integration_test*/ true);

    ForcePermissionRulesForTesting();
  }

  void SetUpMocks() override {
    const URLResponseMap url_responses = {
        {"/v9/catalog",
         {{net::HTTP_OK, "/catalog_with_new_tab_page_ad.json"}}}};
    MockUrlResponses(ads_client_mock_, url_responses);
  }
};

TEST_F(BatAdsNewTabPageAdIntegrationTest, Serve) {
  // Arrange

  // Act
  GetAds()->MaybeServeNewTabPageAd(
      base::BindOnce([](const absl::optional<NewTabPageAdInfo>& ad) {
        // Assert
        EXPECT_TRUE(ad);
        EXPECT_TRUE(ad->IsValid());
        EXPECT_EQ(1, GetAdEventCount(AdType::kNewTabPageAd,
                                     ConfirmationType::kServed));
      }));
}

TEST_F(BatAdsNewTabPageAdIntegrationTest, TriggerServedEvent) {
  // Arrange

  // Act
  GetAds()->TriggerNewTabPageAdEvent(kPlacementId, kCreativeInstanceIdId,
                                     mojom::NewTabPageAdEventType::kServed);

  // Assert
  EXPECT_EQ(1,
            GetAdEventCount(AdType::kNewTabPageAd, ConfirmationType::kServed));
  EXPECT_EQ(0, GetHistoryItemCount());
  EXPECT_EQ(0, GetTransactionCount());
}

TEST_F(BatAdsNewTabPageAdIntegrationTest, TriggerViewedEvent) {
  // Arrange
  GetAds()->TriggerNewTabPageAdEvent(kPlacementId, kCreativeInstanceIdId,
                                     mojom::NewTabPageAdEventType::kServed);

  // Act
  GetAds()->TriggerNewTabPageAdEvent(kPlacementId, kCreativeInstanceIdId,
                                     mojom::NewTabPageAdEventType::kViewed);

  // Assert
  EXPECT_EQ(1,
            GetAdEventCount(AdType::kNewTabPageAd, ConfirmationType::kServed));
  EXPECT_EQ(1,
            GetAdEventCount(AdType::kNewTabPageAd, ConfirmationType::kViewed));
  EXPECT_EQ(1, GetHistoryItemCount());
  EXPECT_EQ(1, GetTransactionCount());
}

TEST_F(BatAdsNewTabPageAdIntegrationTest, TriggerClickedEvent) {
  // Arrange
  GetAds()->TriggerNewTabPageAdEvent(kPlacementId, kCreativeInstanceIdId,
                                     mojom::NewTabPageAdEventType::kServed);
  GetAds()->TriggerNewTabPageAdEvent(kPlacementId, kCreativeInstanceIdId,
                                     mojom::NewTabPageAdEventType::kViewed);

  // Act
  GetAds()->TriggerNewTabPageAdEvent(kPlacementId, kCreativeInstanceIdId,
                                     mojom::NewTabPageAdEventType::kClicked);

  // Assert
  EXPECT_EQ(1,
            GetAdEventCount(AdType::kNewTabPageAd, ConfirmationType::kServed));
  EXPECT_EQ(1,
            GetAdEventCount(AdType::kNewTabPageAd, ConfirmationType::kViewed));
  EXPECT_EQ(1,
            GetAdEventCount(AdType::kNewTabPageAd, ConfirmationType::kClicked));
  EXPECT_EQ(2, GetHistoryItemCount());
  EXPECT_EQ(2, GetTransactionCount());
}

}  // namespace ads
