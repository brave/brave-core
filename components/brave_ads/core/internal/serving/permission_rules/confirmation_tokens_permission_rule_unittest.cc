/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/permission_rules/confirmation_tokens_permission_rule.h"

#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_tokens_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/settings/settings_unittest_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsConfirmationTokensPermissionRuleTest : public UnitTestBase {
 protected:
  const ConfirmationTokensPermissionRule permission_rule_;
};

TEST_F(BraveAdsConfirmationTokensPermissionRuleTest,
       ShouldAllowIfDoesNotExceedCap) {
  // Arrange
  test::SetConfirmationTokens(/*count=*/10);

  // Act & Assert
  EXPECT_TRUE(permission_rule_.ShouldAllow().has_value());
}

TEST_F(BraveAdsConfirmationTokensPermissionRuleTest,
       ShouldAllowIfUserHasNotJoinedBraveRewards) {
  // Arrange
  test::DisableBraveRewards();

  // Act & Assert
  EXPECT_TRUE(permission_rule_.ShouldAllow().has_value());
}

TEST_F(BraveAdsConfirmationTokensPermissionRuleTest,
       ShouldNotAllowIfNoConfirmationTokens) {
  // Act & Assert
  EXPECT_FALSE(permission_rule_.ShouldAllow().has_value());
}

TEST_F(BraveAdsConfirmationTokensPermissionRuleTest,
       ShouldNotAllowIfExceedsCap) {
  // Arrange
  test::SetConfirmationTokens(/*count=*/9);

  // Act & Assert
  EXPECT_FALSE(permission_rule_.ShouldAllow().has_value());
}

}  // namespace brave_ads
