/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/ad_type.h"
#include "brave/components/brave_ads/core/confirmation_type.h"
#include "brave/components/brave_ads/core/inline_content_ad_info.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions_unittest_util.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/ad_event_unittest_util.h"
#include "brave/components/brave_ads/core/internal/ads/ad_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/ads/serving/permission_rules/permission_rules_unittest_util.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_url_request_builder_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_mock_util.h"
#include "brave/components/brave_ads/core/internal/history/history_unittest_util.h"
#include "brave/components/brave_ads/core/internal/privacy/p2a/impressions/p2a_impression.h"
#include "net/http/http_status_code.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

using ::testing::_;

namespace {
constexpr char kDimensions[] = "200x100";
}  // namespace

class BraveAdsInlineContentAdIntegrationTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUpForTesting(/*is_integration_test*/ true);

    ForcePermissionRulesForTesting();
  }

  void SetUpMocks() override {
    const URLResponseMap url_responses = {
        {BuildCatalogUrlPath(),
         {{net::HTTP_OK,
           /*response_body*/ "/catalog_with_inline_content_ad.json"}}}};
    MockUrlResponses(ads_client_mock_, url_responses);
  }
};

TEST_F(BraveAdsInlineContentAdIntegrationTest, Serve) {
  // Arrange

  // Act
  GetAds().MaybeServeInlineContentAd(
      kDimensions,
      base::BindOnce([](const std::string& dimensions,
                        const absl::optional<InlineContentAdInfo>& ad) {
        // Assert
        EXPECT_EQ(kDimensions, dimensions);
        EXPECT_TRUE(ad);
        EXPECT_TRUE(ad->IsValid());
        EXPECT_EQ(1U, GetAdEventCount(AdType::kInlineContentAd,
                                      ConfirmationType::kServed));
      }));
}

TEST_F(BraveAdsInlineContentAdIntegrationTest, TriggerServedEvent) {
  // Arrange

  // Act
  GetAds().TriggerInlineContentAdEvent(
      kPlacementId, kCreativeInstanceId,
      mojom::InlineContentAdEventType::kServed);

  // Assert
  EXPECT_EQ(
      1U, GetAdEventCount(AdType::kInlineContentAd, ConfirmationType::kServed));
  EXPECT_EQ(0U, GetHistoryItemCount());
  EXPECT_EQ(0U, GetTransactionCount());
}

TEST_F(BraveAdsInlineContentAdIntegrationTest, TriggerViewedEvent) {
  // Arrange
  const std::string name =
      privacy::p2a::GetAdImpressionNameForAdType(AdType::kInlineContentAd);
  EXPECT_CALL(ads_client_mock_, RecordP2AEvent(name, _));

  GetAds().TriggerInlineContentAdEvent(
      kPlacementId, kCreativeInstanceId,
      mojom::InlineContentAdEventType::kServed);

  // Act
  GetAds().TriggerInlineContentAdEvent(
      kPlacementId, kCreativeInstanceId,
      mojom::InlineContentAdEventType::kViewed);

  // Assert
  EXPECT_EQ(
      1U, GetAdEventCount(AdType::kInlineContentAd, ConfirmationType::kViewed));
  EXPECT_EQ(1U, GetHistoryItemCount());
  EXPECT_EQ(1U, GetTransactionCount());
}

TEST_F(BraveAdsInlineContentAdIntegrationTest, TriggerClickedEvent) {
  // Arrange
  GetAds().TriggerInlineContentAdEvent(
      kPlacementId, kCreativeInstanceId,
      mojom::InlineContentAdEventType::kServed);
  GetAds().TriggerInlineContentAdEvent(
      kPlacementId, kCreativeInstanceId,
      mojom::InlineContentAdEventType::kViewed);

  // Act
  GetAds().TriggerInlineContentAdEvent(
      kPlacementId, kCreativeInstanceId,
      mojom::InlineContentAdEventType::kClicked);

  // Assert
  EXPECT_EQ(
      1U, GetAdEventCount(AdType::kInlineContentAd, ConfirmationType::kServed));
  EXPECT_EQ(
      1U, GetAdEventCount(AdType::kInlineContentAd, ConfirmationType::kViewed));
  EXPECT_EQ(1U, GetAdEventCount(AdType::kInlineContentAd,
                                ConfirmationType::kClicked));
  EXPECT_EQ(2U, GetHistoryItemCount());
  EXPECT_EQ(2U, GetTransactionCount());
}

}  // namespace brave_ads
