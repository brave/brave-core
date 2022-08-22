/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/search_result_ad.h"

#include "bat/ads/ad_type.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/account/transactions/transactions_unittest_util.h"
#include "bat/ads/internal/ads/ad_events/ad_event_unittest_util.h"
#include "bat/ads/internal/ads/serving/permission_rules/permission_rules_unittest_util.h"
#include "bat/ads/internal/base/unittest/unittest_base.h"
#include "bat/ads/internal/creatives/search_result_ads/search_result_ad_unittest_util.h"
#include "bat/ads/internal/history/history_unittest_util.h"
#include "bat/ads/internal/privacy/tokens/unblinded_tokens/unblinded_tokens_unittest_util.h"
#include "bat/ads/public/interfaces/ads.mojom.h"

// npm run test -- brave_unit_tests --filter=BatAds*

using testing::_;

namespace ads {

class BatAdsSearchResultAdTest : public UnitTestBase {
 protected:
  BatAdsSearchResultAdTest() = default;

  ~BatAdsSearchResultAdTest() override = default;

  void SetUp() override {
    UnitTestBase::SetUpForTesting(/* is_integration_test */ true);

    ForcePermissionRules();

    // Need to trigger several search result ad events.
    privacy::SetUnblindedTokens(11);
  }
};

TEST_F(BatAdsSearchResultAdTest, TriggerViewedEvents) {
  // Arrange

  // Act
  GetAds()->TriggerSearchResultAdEvent(BuildSearchResultAd(),
                                       mojom::SearchResultAdEventType::kViewed);

  GetAds()->TriggerSearchResultAdEvent(BuildSearchResultAd(),
                                       mojom::SearchResultAdEventType::kViewed);

  // Assert
  EXPECT_EQ(
      2, GetAdEventCount(AdType::kSearchResultAd, ConfirmationType::kServed));
  EXPECT_EQ(
      2, GetAdEventCount(AdType::kSearchResultAd, ConfirmationType::kViewed));
  EXPECT_EQ(2, GetHistoryItemCount());
  EXPECT_EQ(2, GetTransactionCount());
}

TEST_F(BatAdsSearchResultAdTest, TriggerQueuedViewedEvents) {
  // Arrange

  // Act
  SearchResultAd::DeferTriggeringOfAdViewedEventForTesting();
  // This ad viewed event triggering will be deferred.
  GetAds()->TriggerSearchResultAdEvent(BuildSearchResultAd(),
                                       mojom::SearchResultAdEventType::kViewed);

  // This ad viewed event will be queued as the previous ad viewed event has not
  // completed.
  GetAds()->TriggerSearchResultAdEvent(BuildSearchResultAd(),
                                       mojom::SearchResultAdEventType::kViewed);

  EXPECT_EQ(
      1, GetAdEventCount(AdType::kSearchResultAd, ConfirmationType::kServed));
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

TEST_F(BatAdsSearchResultAdTest, TriggerClickedEvent) {
  // Arrange

  // Act
  GetAds()->TriggerSearchResultAdEvent(
      BuildSearchResultAd(), mojom::SearchResultAdEventType::kClicked);

  // Assert
  EXPECT_EQ(
      1, GetAdEventCount(AdType::kSearchResultAd, ConfirmationType::kClicked));
  EXPECT_EQ(1, GetHistoryItemCount());
  EXPECT_EQ(1, GetTransactionCount());
}

}  // namespace ads
