/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/permission_rules/unblinded_tokens_frequency_cap.h"

#include "bat/ads/internal/privacy/unblinded_tokens/unblinded_tokens_unittest_util.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsUnblindedTokensFrequencyCapTest : public UnitTestBase {
 protected:
  BatAdsUnblindedTokensFrequencyCapTest() = default;

  ~BatAdsUnblindedTokensFrequencyCapTest() override = default;
};

TEST_F(BatAdsUnblindedTokensFrequencyCapTest, AllowAdIfDoesNotExceedCap) {
  // Arrange
  privacy::SetUnblindedTokens(10);

  // Act
  UnblindedTokensFrequencyCap frequency_cap;
  const bool is_allowed = frequency_cap.ShouldAllow();

  // Assert
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsUnblindedTokensFrequencyCapTest, DoNotAllowAdIfNoUnblindedTokens) {
  // Arrange

  // Act
  UnblindedTokensFrequencyCap frequency_cap;
  const bool is_allowed = frequency_cap.ShouldAllow();

  // Assert
  EXPECT_FALSE(is_allowed);
}

TEST_F(BatAdsUnblindedTokensFrequencyCapTest, DoNotAllowAdIfExceedsCap) {
  // Arrange
  privacy::SetUnblindedTokens(9);

  // Act
  UnblindedTokensFrequencyCap frequency_cap;
  const bool is_allowed = frequency_cap.ShouldAllow();

  // Assert
  EXPECT_FALSE(is_allowed);
}

}  // namespace ads
