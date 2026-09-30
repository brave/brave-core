/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/test/test_future.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/creatives/search_result_ads/test/creative_search_result_ad_test_util.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/test/permission_rules_test_util.h"
#include "brave/components/brave_ads/core/internal/settings/test/settings_test_util.h"
#include "brave/components/brave_ads/core/public/ads.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsSearchResultAdForNonRewardsIntegrationTest
    : public test::TestBase {
 public:
  BraveAdsSearchResultAdForNonRewardsIntegrationTest()
      : test::TestBase(/*is_integration_test=*/true) {}

 protected:
  void SetUp() override {
    test::TestBase::SetUp();

    test::ForcePermissionRules();

    test::DisableBraveRewards();
  }

  void TriggerSearchResultAdEventAndVerifyExpectations(
      mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad,
      mojom::SearchResultAdEventType mojom_ad_event_type,
      bool should_fire_event) {
    base::test::TestFuture<bool> test_future;
    GetAds().TriggerSearchResultAdEvent(std::move(mojom_creative_ad),
                                        mojom_ad_event_type,
                                        test_future.GetCallback());
    EXPECT_EQ(should_fire_event, test_future.Get());
  }
};

TEST_F(BraveAdsSearchResultAdForNonRewardsIntegrationTest,
       DoNotTriggerViewedEvent) {
  // Act & Assert
  TriggerSearchResultAdEventAndVerifyExpectations(
      test::BuildCreativeSearchResultAdWithConversion(
          /*use_random_uuids=*/true),
      mojom::SearchResultAdEventType::kViewedImpression,
      /*should_fire_event=*/false);
}

TEST_F(BraveAdsSearchResultAdForNonRewardsIntegrationTest,
       TriggerClickedEvent) {
  // Arrange
  const mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad =
      test::BuildCreativeSearchResultAdWithConversion(
          /*use_random_uuids=*/true);

  // Act & Assert
  TriggerSearchResultAdEventAndVerifyExpectations(
      mojom_creative_ad.Clone(), mojom::SearchResultAdEventType::kClicked,
      /*should_fire_event=*/true);
}

TEST_F(BraveAdsSearchResultAdForNonRewardsIntegrationTest,
       DoNotTriggerDuplicateClickedEvent) {
  // Arrange
  const mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad =
      test::BuildCreativeSearchResultAdWithConversion(
          /*use_random_uuids=*/true);

  // Act
  base::test::TestFuture<bool> clicked_future;
  GetAds().TriggerSearchResultAdEvent(mojom_creative_ad.Clone(),
                                      mojom::SearchResultAdEventType::kClicked,
                                      clicked_future.GetCallback());

  base::test::TestFuture<bool> duplicate_clicked_future;
  GetAds().TriggerSearchResultAdEvent(mojom_creative_ad.Clone(),
                                      mojom::SearchResultAdEventType::kClicked,
                                      duplicate_clicked_future.GetCallback());

  // Assert
  EXPECT_TRUE(clicked_future.Get());
  EXPECT_FALSE(duplicate_clicked_future.Get());
}

}  // namespace brave_ads
