/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_token_info.h"

#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/test/confirmation_tokens_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsConfirmationTokenInfoTest : public test::TestBase {};

TEST_F(BraveAdsConfirmationTokenInfoTest, IsValid) {
  // Act & Assert
  EXPECT_TRUE(test::BuildConfirmationToken().IsValid());
}

TEST_F(BraveAdsConfirmationTokenInfoTest, IsNotValid) {
  // Arrange
  const ConfirmationTokenInfo confirmation_token;

  // Act & Assert
  EXPECT_FALSE(confirmation_token.IsValid());
}

}  // namespace brave_ads
