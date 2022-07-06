/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/search_result_ad.h"

#include "base/notreached.h"
#include "bat/ads/ad_type.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/account/transactions/transactions_unittest_util.h"
#include "bat/ads/internal/ads/ad_events/ad_event_unittest_util.h"
#include "bat/ads/internal/ads/serving/permission_rules/permission_rules_unittest_util.h"
#include "bat/ads/internal/base/unittest/unittest_base.h"
#include "bat/ads/internal/creatives/search_result_ads/search_result_ad_unittest_util.h"
#include "bat/ads/internal/history/history_unittest_util.h"
#include "bat/ads/public/interfaces/ads.mojom.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsSearchResultAdIntegrationTest : public UnitTestBase {
 protected:
  BatAdsSearchResultAdIntegrationTest() = default;

  ~BatAdsSearchResultAdIntegrationTest() override = default;

  void SetUp() override {
    UnitTestBase::SetUpForTesting(/* is_integration_test */ true);

    ForcePermissionRules();
  }
};

TEST_F(BatAdsSearchResultAdIntegrationTest, TriggerViewedEvent) {
  // Arrange

  // Act
  GetAds()->TriggerSearchResultAdEvent(
      BuildSearchResultAd(), mojom::SearchResultAdEventType::kViewed,
      [=](const bool success, const std::string& placement_id,
          const mojom::SearchResultAdEventType event_type) {
        // Assert
        ASSERT_TRUE(success);

        switch (event_type) {
          case mojom::SearchResultAdEventType::kServed: {
            EXPECT_EQ(1, GetAdEventCount(AdType::kSearchResultAd,
                                         ConfirmationType::kServed));
            EXPECT_EQ(0, GetHistoryItemCount());
            EXPECT_EQ(0, GetTransactionCount());
            break;
          }

          case mojom::SearchResultAdEventType::kViewed: {
            EXPECT_EQ(1, GetAdEventCount(AdType::kSearchResultAd,
                                         ConfirmationType::kViewed));
            EXPECT_EQ(1, GetHistoryItemCount());
            EXPECT_EQ(1, GetTransactionCount());
            break;
          }

          default: {
            NOTREACHED();
            break;
          }
        }
      });
}

TEST_F(BatAdsSearchResultAdIntegrationTest, TriggerClickedEvent) {
  // Arrange

  // Act
  GetAds()->TriggerSearchResultAdEvent(
      BuildSearchResultAd(), mojom::SearchResultAdEventType::kClicked,
      [=](const bool success, const std::string& placement_id,
          const mojom::SearchResultAdEventType event_type) {
        // Assert
        ASSERT_TRUE(success);

        EXPECT_EQ(mojom::SearchResultAdEventType::kClicked, event_type);
        EXPECT_EQ(1, GetAdEventCount(AdType::kSearchResultAd,
                                     ConfirmationType::kClicked));
        EXPECT_EQ(1, GetHistoryItemCount());
        EXPECT_EQ(1, GetTransactionCount());
      });
}

}  // namespace ads
