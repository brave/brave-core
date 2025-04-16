/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// npm run test -- brave_unit_tests --filter=BraveAds*

#include "brave/components/brave_ads/core/internal/account/account_util.h"

#include <cstddef>

#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_test_constants.h"
#include "brave/components/brave_ads/core/internal/ads_core/ads_core_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/settings/settings_test_util.h"
#include "brave/components/brave_ads/core/public/ads_feature.h"

namespace brave_ads {

class BraveAdsAccountUtilTest : public test::TestBase {};

TEST_F(BraveAdsAccountUtilTest, AlwaysAllowDepositsForRewardsUser) {
  // Act & Assert
  for (int i = 0; i < static_cast<int>(mojom::AdType::kMaxValue); ++i) {
    for (int j = 0; j < static_cast<int>(mojom::ConfirmationType::kMaxValue);
         ++j) {
      EXPECT_TRUE(IsAllowedToDeposit(test::kCreativeInstanceId,
                                     static_cast<mojom::AdType>(i),
                                     static_cast<mojom::ConfirmationType>(j)));
    }
  }
}

TEST_F(BraveAdsAccountUtilTest, AllowInlineContentAdDepositsForNonRewardsUser) {
  // Arrange
  test::DisableBraveRewards();

  // Act & Assert
  for (int i = 0; i < static_cast<int>(mojom::ConfirmationType::kMaxValue);
       ++i) {
    EXPECT_TRUE(IsAllowedToDeposit(test::kCreativeInstanceId,
                                   mojom::AdType::kInlineContentAd,
                                   static_cast<mojom::ConfirmationType>(i)));
  }
}

TEST_F(
    BraveAdsAccountUtilTest,
    DoNotAllowInlineContentAdDepositsForNonRewardsUserIfOptedOutOfBraveNews) {
  // Arrange
  test::DisableBraveRewards();

  test::OptOutOfBraveNewsAds();

  // Act & Assert
  for (int i = 0; i < static_cast<int>(mojom::ConfirmationType::kMaxValue);
       ++i) {
    EXPECT_FALSE(IsAllowedToDeposit(test::kCreativeInstanceId,
                                    mojom::AdType::kInlineContentAd,
                                    static_cast<mojom::ConfirmationType>(i)));
  }
}

TEST_F(BraveAdsAccountUtilTest,
       AllowPromotedContentAdDepositsForNonRewardsUser) {
  // Arrange
  test::DisableBraveRewards();

  // Act & Assert
  for (int i = 0; i < static_cast<int>(mojom::ConfirmationType::kMaxValue);
       ++i) {
    EXPECT_TRUE(IsAllowedToDeposit(test::kCreativeInstanceId,
                                   mojom::AdType::kPromotedContentAd,
                                   static_cast<mojom::ConfirmationType>(i)));
  }
}

TEST_F(
    BraveAdsAccountUtilTest,
    DoNotAllowPromotedContentAdDepositsForNonRewardsUserIfOptedOutOfBraveNews) {
  // Arrange
  test::DisableBraveRewards();

  test::OptOutOfBraveNewsAds();

  // Act & Assert
  for (int i = 0; i < static_cast<int>(mojom::ConfirmationType::kMaxValue);
       ++i) {
    EXPECT_FALSE(IsAllowedToDeposit(test::kCreativeInstanceId,
                                    mojom::AdType::kPromotedContentAd,
                                    static_cast<mojom::ConfirmationType>(i)));
  }
}

TEST_F(BraveAdsAccountUtilTest, AllowNewTabPageAdDepositsForNonRewardsUser) {
  // Arrange
  test::DisableBraveRewards();

  // Act & Assert
  for (int i = 0; i < static_cast<int>(mojom::ConfirmationType::kMaxValue);
       ++i) {
    EXPECT_TRUE(IsAllowedToDeposit(test::kCreativeInstanceId,
                                   mojom::AdType::kNewTabPageAd,
                                   static_cast<mojom::ConfirmationType>(i)));
  }
}

TEST_F(
    BraveAdsAccountUtilTest,
    DoNotAllowNewTabPageAdDepositsForNonRewardsUserIfOptedOutOfNewTabPageAds) {
  // Arrange
  test::DisableBraveRewards();

  test::OptOutOfNewTabPageAds();

  // Act & Assert
  for (int i = 0; i < static_cast<int>(mojom::ConfirmationType::kMaxValue);
       ++i) {
    EXPECT_FALSE(IsAllowedToDeposit(test::kCreativeInstanceId,
                                    mojom::AdType::kNewTabPageAd,
                                    static_cast<mojom::ConfirmationType>(i)));
  }
}

TEST_F(BraveAdsAccountUtilTest,
       DoNotAllowNewTabPageAdDepositsForNonRewardsUserIfShouldFallbackToP3A) {
  // Arrange
  test::DisableBraveRewards();

  SetCreativeInstanceIdsToFallbackToP3a({test::kCreativeInstanceId});

  // Act & Assert
  for (int i = 0; i < static_cast<int>(mojom::ConfirmationType::kMaxValue);
       ++i) {
    EXPECT_FALSE(IsAllowedToDeposit(test::kCreativeInstanceId,
                                    mojom::AdType::kNewTabPageAd,
                                    static_cast<mojom::ConfirmationType>(i)));
  }
}

TEST_F(BraveAdsAccountUtilTest,
       DoNotAllowNotificationAdDepositsForNonRewardsUser) {
  // Arrange
  test::DisableBraveRewards();

  // Act & Assert
  for (int i = 0; i < static_cast<int>(mojom::ConfirmationType::kMaxValue);
       ++i) {
    EXPECT_FALSE(IsAllowedToDeposit(test::kCreativeInstanceId,
                                    mojom::AdType::kNotificationAd,
                                    static_cast<mojom::ConfirmationType>(i)));
  }
}

TEST_F(BraveAdsAccountUtilTest,
       OnlyAllowSearchResultAdConversionDepositForNonRewardsUser) {
  // Arrange
  const base::test::ScopedFeatureList scoped_feature_list(
      {kShouldAlwaysTriggerBraveSearchResultAdEventsFeature});

  test::DisableBraveRewards();

  // Act & Assert
  for (int i = 0; i < static_cast<int>(mojom::ConfirmationType::kMaxValue);
       ++i) {
    const auto confirmation_type = static_cast<mojom::ConfirmationType>(i);

    const bool is_allowed_to_deposit =
        IsAllowedToDeposit(test::kCreativeInstanceId,
                           mojom::AdType::kSearchResultAd, confirmation_type);

    if (confirmation_type == mojom::ConfirmationType::kConversion) {
      EXPECT_TRUE(is_allowed_to_deposit);
    } else {
      EXPECT_FALSE(is_allowed_to_deposit);
    }
  }
}

}  // namespace brave_ads
