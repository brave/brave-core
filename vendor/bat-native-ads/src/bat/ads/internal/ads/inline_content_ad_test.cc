/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "absl/types/optional.h"
#include "base/functional/bind.h"
#include "bat/ads/ad_type.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/inline_content_ad_info.h"
#include "bat/ads/internal/account/transactions/transactions_unittest_util.h"
#include "bat/ads/internal/ads/ad_events/ad_event_unittest_util.h"
#include "bat/ads/internal/ads/serving/permission_rules/permission_rules_unittest_util.h"
#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/common/unittest/unittest_mock_util.h"
#include "bat/ads/internal/history/history_unittest_util.h"
#include "bat/ads/internal/privacy/p2a/impressions/p2a_impression.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

using ::testing::_;

namespace {

constexpr char kDimensions[] = "200x100";
constexpr char kPlacementId[] = "f0948316-df6f-4e31-814d-d0b5f2a1f28c";
constexpr char kCreativeInstanceIdId[] = "30db5f7b-dba3-48a3-b299-c9bd9c67da65";

}  // namespace

class BatAdsInlineContentAdIntegrationTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUpForTesting(/*is_integration_test*/ true);

    ForcePermissionRulesForTesting();
  }

  void SetUpMocks() override {
    const URLResponseMap url_responses = {
        {"/v9/catalog",
         {{net::HTTP_OK, "/catalog_with_inline_content_ad.json"}}}};
    MockUrlResponses(ads_client_mock_, url_responses);
  }
};

TEST_F(BatAdsInlineContentAdIntegrationTest, Serve) {
  // Arrange

  // Act
  GetAds()->MaybeServeInlineContentAd(
      kDimensions,
      base::BindOnce([](const std::string& dimensions,
                        const absl::optional<InlineContentAdInfo>& ad) {
        // Assert
        EXPECT_EQ(kDimensions, dimensions);
        EXPECT_TRUE(ad);
        EXPECT_TRUE(ad->IsValid());
        EXPECT_EQ(1, GetAdEventCount(AdType::kInlineContentAd,
                                     ConfirmationType::kServed));
      }));
}

TEST_F(BatAdsInlineContentAdIntegrationTest, TriggerServedEvent) {
  // Arrange

  // Act
  GetAds()->TriggerInlineContentAdEvent(
      kPlacementId, kCreativeInstanceIdId,
      mojom::InlineContentAdEventType::kServed);

  // Assert
  EXPECT_EQ(
      1, GetAdEventCount(AdType::kInlineContentAd, ConfirmationType::kServed));
  EXPECT_EQ(0, GetHistoryItemCount());
  EXPECT_EQ(0, GetTransactionCount());
}

TEST_F(BatAdsInlineContentAdIntegrationTest, TriggerViewedEvent) {
  // Arrange
  const std::string name =
      privacy::p2a::GetAdImpressionNameForAdType(AdType::kInlineContentAd);
  EXPECT_CALL(*ads_client_mock_, RecordP2AEvent(name, _));

  GetAds()->TriggerInlineContentAdEvent(
      kPlacementId, kCreativeInstanceIdId,
      mojom::InlineContentAdEventType::kServed);

  // Act
  GetAds()->TriggerInlineContentAdEvent(
      kPlacementId, kCreativeInstanceIdId,
      mojom::InlineContentAdEventType::kViewed);

  // Assert
  EXPECT_EQ(
      1, GetAdEventCount(AdType::kInlineContentAd, ConfirmationType::kViewed));
  EXPECT_EQ(1, GetHistoryItemCount());
  EXPECT_EQ(1, GetTransactionCount());
}

TEST_F(BatAdsInlineContentAdIntegrationTest, TriggerClickedEvent) {
  // Arrange
  GetAds()->TriggerInlineContentAdEvent(
      kPlacementId, kCreativeInstanceIdId,
      mojom::InlineContentAdEventType::kServed);
  GetAds()->TriggerInlineContentAdEvent(
      kPlacementId, kCreativeInstanceIdId,
      mojom::InlineContentAdEventType::kViewed);

  // Act
  GetAds()->TriggerInlineContentAdEvent(
      kPlacementId, kCreativeInstanceIdId,
      mojom::InlineContentAdEventType::kClicked);

  // Assert
  EXPECT_EQ(
      1, GetAdEventCount(AdType::kInlineContentAd, ConfirmationType::kServed));
  EXPECT_EQ(
      1, GetAdEventCount(AdType::kInlineContentAd, ConfirmationType::kViewed));
  EXPECT_EQ(
      1, GetAdEventCount(AdType::kInlineContentAd, ConfirmationType::kClicked));
  EXPECT_EQ(2, GetHistoryItemCount());
  EXPECT_EQ(2, GetTransactionCount());
}

}  // namespace ads
