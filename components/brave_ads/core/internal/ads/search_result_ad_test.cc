/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/search_result_ad_handler.h"

#include "brave/components/brave_ads/core/ad_type.h"
#include "brave/components/brave_ads/core/confirmation_type.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions_unittest_util.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/ad_event_unittest_util.h"
#include "brave/components/brave_ads/core/internal/ads/serving/permission_rules/permission_rules_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/creatives/search_result_ads/search_result_ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/history/history_unittest_util.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/unblinded_tokens/unblinded_tokens_unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace brave_ads {

class BatAdsSearchResultAdIntegrationTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUpForTesting(/*is_integration_test*/ true);

    ForcePermissionRulesForTesting();

    // Need to trigger several search result ad events.
    privacy::SetUnblindedTokens(11);
  }
};

TEST_F(BatAdsSearchResultAdIntegrationTest, TriggerViewedEvents) {
  // Arrange

  // Act
  {
    const mojom::SearchResultAdInfoPtr search_result_ad = BuildSearchResultAd();

    GetAds()->TriggerSearchResultAdEvent(
        search_result_ad.Clone(), mojom::SearchResultAdEventType::kServed);
    GetAds()->TriggerSearchResultAdEvent(
        search_result_ad.Clone(), mojom::SearchResultAdEventType::kViewed);
  }

  {
    const mojom::SearchResultAdInfoPtr search_result_ad = BuildSearchResultAd();

    GetAds()->TriggerSearchResultAdEvent(
        search_result_ad.Clone(), mojom::SearchResultAdEventType::kServed);
    GetAds()->TriggerSearchResultAdEvent(
        search_result_ad.Clone(), mojom::SearchResultAdEventType::kViewed);
  }

  // Assert
  EXPECT_EQ(
      2, GetAdEventCount(AdType::kSearchResultAd, ConfirmationType::kServed));
  EXPECT_EQ(
      2, GetAdEventCount(AdType::kSearchResultAd, ConfirmationType::kViewed));
  EXPECT_EQ(2, GetHistoryItemCount());
  EXPECT_EQ(2, GetTransactionCount());
}

TEST_F(BatAdsSearchResultAdIntegrationTest, TriggerQueuedViewedEvents) {
  // Arrange

  // Act
  SearchResultAd::DeferTriggeringOfAdViewedEventForTesting();

  {
    // This ad viewed event triggering will be deferred.
    const mojom::SearchResultAdInfoPtr search_result_ad = BuildSearchResultAd();

    GetAds()->TriggerSearchResultAdEvent(
        search_result_ad.Clone(), mojom::SearchResultAdEventType::kServed);
    GetAds()->TriggerSearchResultAdEvent(
        search_result_ad.Clone(), mojom::SearchResultAdEventType::kViewed);
  }

  {
    // This ad viewed event will be queued as the previous ad viewed event has
    // not completed.
    const mojom::SearchResultAdInfoPtr search_result_ad = BuildSearchResultAd();

    GetAds()->TriggerSearchResultAdEvent(
        search_result_ad.Clone(), mojom::SearchResultAdEventType::kServed);
    GetAds()->TriggerSearchResultAdEvent(
        search_result_ad.Clone(), mojom::SearchResultAdEventType::kViewed);
  }

  EXPECT_EQ(
      2, GetAdEventCount(AdType::kSearchResultAd, ConfirmationType::kServed));
  EXPECT_EQ(
      1, GetAdEventCount(AdType::kSearchResultAd, ConfirmationType::kViewed));
  EXPECT_EQ(1, GetHistoryItemCount());
  EXPECT_EQ(1, GetTransactionCount());

  // Complete triggering of the deferred ad viewed event.
  SearchResultAd::TriggerDeferredAdViewedEventForTesting();

  // Assert
  EXPECT_EQ(
      2, GetAdEventCount(AdType::kSearchResultAd, ConfirmationType::kServed));
  EXPECT_EQ(
      2, GetAdEventCount(AdType::kSearchResultAd, ConfirmationType::kViewed));
  EXPECT_EQ(2, GetHistoryItemCount());
  EXPECT_EQ(2, GetTransactionCount());
}

TEST_F(BatAdsSearchResultAdIntegrationTest, TriggerClickedEvent) {
  // Arrange
  const mojom::SearchResultAdInfoPtr search_result_ad = BuildSearchResultAd();

  GetAds()->TriggerSearchResultAdEvent(search_result_ad->Clone(),
                                       mojom::SearchResultAdEventType::kServed);
  GetAds()->TriggerSearchResultAdEvent(search_result_ad->Clone(),
                                       mojom::SearchResultAdEventType::kViewed);

  // Act
  GetAds()->TriggerSearchResultAdEvent(
      search_result_ad->Clone(), mojom::SearchResultAdEventType::kClicked);

  // Assert
  EXPECT_EQ(
      1, GetAdEventCount(AdType::kSearchResultAd, ConfirmationType::kServed));
  EXPECT_EQ(
      1, GetAdEventCount(AdType::kSearchResultAd, ConfirmationType::kViewed));
  EXPECT_EQ(
      1, GetAdEventCount(AdType::kSearchResultAd, ConfirmationType::kClicked));
  EXPECT_EQ(2, GetHistoryItemCount());
  EXPECT_EQ(2, GetTransactionCount());
}

}  // namespace brave_ads
