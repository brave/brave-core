/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/inline_content_ad.h"

#include "bat/ads/ad_type.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/inline_content_ad_info.h"
#include "bat/ads/internal/account/transactions/transactions_unittest_util.h"
#include "bat/ads/internal/ads/ad_events/ad_event_unittest_util.h"
#include "bat/ads/internal/ads/serving/permission_rules/permission_rules_unittest_util.h"
#include "bat/ads/internal/base/net/http/http_status_code.h"
#include "bat/ads/internal/base/unittest/unittest_base.h"
#include "bat/ads/internal/base/unittest/unittest_mock_util.h"
#include "bat/ads/internal/creatives/inline_content_ads/creative_inline_content_ad_unittest_util.h"
#include "bat/ads/internal/history/history_unittest_util.h"
#include "bat/ads/internal/privacy/p2a/impressions/p2a_impression.h"
#include "bat/ads/public/interfaces/ads.mojom.h"

// npm run test -- brave_unit_tests --filter=BatAds*

using ::testing::_;

namespace ads {

namespace {

constexpr char kDimensions[] = "200x100";
constexpr char kPlacementId[] = "f0948316-df6f-4e31-814d-d0b5f2a1f28c";
constexpr char kCreativeInstanceIdId[] = "30db5f7b-dba3-48a3-b299-c9bd9c67da65";

}  // namespace

class BatAdsInlineContentAdIntegrationTest : public UnitTestBase {
 protected:
  BatAdsInlineContentAdIntegrationTest() = default;

  ~BatAdsInlineContentAdIntegrationTest() override = default;

  void SetUp() override {
    UnitTestBase::SetUpForTesting(/* is_integration_test */ true);

    ForcePermissionRules();
  }

  void SetUpMocks() override {
    const URLEndpointMap endpoints = {
        {"/v9/catalog",
         {{net::HTTP_OK, "/catalog_with_inline_content_ad.json"}}}};
    MockUrlRequest(ads_client_mock_, endpoints);
  }
};

TEST_F(BatAdsInlineContentAdIntegrationTest, Serve) {
  // Arrange

  // Act
  GetAds()->MaybeServeInlineContentAd(
      kDimensions, [](const bool success, const std::string& dimensions,
                      const InlineContentAdInfo& ad) {
        // Assert
        EXPECT_TRUE(success);
        EXPECT_EQ(kDimensions, dimensions);
        EXPECT_TRUE(ad.IsValid());
        EXPECT_EQ(1, GetAdEventCount(AdType::kInlineContentAd,
                                     ConfirmationType::kServed));
      });
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

  // Act
  GetAds()->TriggerInlineContentAdEvent(
      kPlacementId, kCreativeInstanceIdId,
      mojom::InlineContentAdEventType::kClicked);

  // Assert
  EXPECT_EQ(
      1, GetAdEventCount(AdType::kInlineContentAd, ConfirmationType::kClicked));
  EXPECT_EQ(1, GetHistoryItemCount());
  EXPECT_EQ(1, GetTransactionCount());
}

}  // namespace ads
