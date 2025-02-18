/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// npm run test -- brave_unit_tests --filter=BraveAds*

#include "brave/components/brave_ads/core/internal/account/account_util.h"

#include <cstddef>

#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/settings/settings_test_util.h"
#include "brave/components/brave_ads/core/public/ads_feature.h"

namespace brave_ads {

class BraveAdsAccountUtilTest : public test::TestBase {};

TEST_F(BraveAdsAccountUtilTest, AlwaysAllowDepositsForRewardsUser) {
  // Act & Assert
  for (size_t i = 0; i < static_cast<size_t>(mojom::AdType::kMaxValue); ++i) {
    for (size_t j = 0;
         j < static_cast<size_t>(mojom::ConfirmationType::kMaxValue); ++j) {
      EXPECT_TRUE(IsAllowedToDeposit(static_cast<mojom::AdType>(i),
                                     static_cast<mojom::ConfirmationType>(j)));
    }
  }
}

TEST_F(BraveAdsAccountUtilTest,
       AlwaysAllowInlineContentAdDepositsForNonRewardsUser) {
  // Arrange
  test::DisableBraveRewards();

  // Act & Assert
  for (size_t i = 0;
       i < static_cast<size_t>(mojom::ConfirmationType::kMaxValue); ++i) {
    EXPECT_TRUE(IsAllowedToDeposit(mojom::AdType::kInlineContentAd,
                                   static_cast<mojom::ConfirmationType>(i)));
  }
}

TEST_F(BraveAdsAccountUtilTest,
       AlwaysAllowPromotedContentAdDepositsForNonRewardsUser) {
  // Arrange
  test::DisableBraveRewards();

  // Act & Assert
  for (size_t i = 0;
       i < static_cast<size_t>(mojom::ConfirmationType::kMaxValue); ++i) {
    EXPECT_TRUE(IsAllowedToDeposit(mojom::AdType::kPromotedContentAd,
                                   static_cast<mojom::ConfirmationType>(i)));
  }
}

TEST_F(BraveAdsAccountUtilTest,
       DoNotAllowNewTabPageAdDepositsForNonRewardsUser) {
  // Arrange
  test::DisableBraveRewards();

  // Act & Assert
  for (size_t i = 0;
       i < static_cast<size_t>(mojom::ConfirmationType::kMaxValue); ++i) {
    EXPECT_FALSE(IsAllowedToDeposit(mojom::AdType::kNewTabPageAd,
                                    static_cast<mojom::ConfirmationType>(i)));
  }
}

TEST_F(
    BraveAdsAccountUtilTest,
    AllowNewTabPageAdDepositsForNonRewardsUserIfShouldAlwaysTriggerNewTabPageAdEvents) {
  // Arrange
  const base::test::ScopedFeatureList scoped_feature_list(
      kShouldAlwaysTriggerBraveNewTabPageAdEventsFeature);

  test::DisableBraveRewards();

  // Act & Assert
  for (size_t i = 0;
       i < static_cast<size_t>(mojom::ConfirmationType::kMaxValue); ++i) {
    EXPECT_TRUE(IsAllowedToDeposit(mojom::AdType::kNewTabPageAd,
                                   static_cast<mojom::ConfirmationType>(i)));
  }
}

TEST_F(BraveAdsAccountUtilTest,
       DoNotAllowNotificationAdDepositsForNonRewardsUser) {
  // Arrange
  test::DisableBraveRewards();

  // Act & Assert
  for (size_t i = 0;
       i < static_cast<size_t>(mojom::ConfirmationType::kMaxValue); ++i) {
    EXPECT_FALSE(IsAllowedToDeposit(mojom::AdType::kNotificationAd,
                                    static_cast<mojom::ConfirmationType>(i)));
  }
}

TEST_F(
    BraveAdsAccountUtilTest,
    DoNotAllowSearchResultAdDepositsForNonRewardsUserIfShouldNotAlwaysTriggerSearchResultAdEvents) {
  // Arrange
  test::DisableBraveRewards();

  // Act & Assert
  for (size_t i = 0;
       i < static_cast<size_t>(mojom::ConfirmationType::kMaxValue); ++i) {
    EXPECT_FALSE(IsAllowedToDeposit(mojom::AdType::kSearchResultAd,
                                    static_cast<mojom::ConfirmationType>(i)));
  }
}

TEST_F(
    BraveAdsAccountUtilTest,
    OnlyAllowSearchResultAdConversionDepositForNonRewardsUserIfShouldAlwaysTriggerSearchResultAdEvents) {
  // Arrange
  const base::test::ScopedFeatureList scoped_feature_list(
      kShouldAlwaysTriggerBraveSearchResultAdEventsFeature);

  test::DisableBraveRewards();

  // Act & Assert
  for (size_t i = 0;
       i < static_cast<size_t>(mojom::ConfirmationType::kMaxValue); ++i) {
    const auto confirmation_type = static_cast<mojom::ConfirmationType>(i);

    const bool is_allowed_to_deposit =
        IsAllowedToDeposit(mojom::AdType::kSearchResultAd, confirmation_type);

    if (confirmation_type == mojom::ConfirmationType::kConversion) {
      EXPECT_TRUE(is_allowed_to_deposit);
    } else {
      EXPECT_FALSE(is_allowed_to_deposit);
    }
  }
}

}  // namespace brave_ads
