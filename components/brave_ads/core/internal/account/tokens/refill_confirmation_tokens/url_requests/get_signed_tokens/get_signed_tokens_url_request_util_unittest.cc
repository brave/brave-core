/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/tokens/refill_confirmation_tokens/url_requests/get_signed_tokens/get_signed_tokens_url_request_util.h"

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsGetSignedTokensUrlRequestUtilTest : public UnitTestBase {};

TEST(BraveAdsGetSignedTokensUrlRequestUtilTest, ParseCaptchaId) {
  // Arrange

  // Act

  // Assert
  FAIL();
}

TEST(BraveAdsGetSignedTokensUrlRequestUtilTest, DoNotParseMissingCaptchaId) {
  // Arrange

  // Act

  // Assert
  FAIL();
}

TEST(BraveAdsGetSignedTokensUrlRequestUtilTest, ParseAndUnblindSignedTokens) {
  // Arrange

  // Act

  // Assert
  FAIL();
}

TEST(BraveAdsGetSignedTokensUrlRequestUtilTest,
     DoNotParseAndUnblindSignedTokensIfMissingBatchDLEQProof) {
  // Arrange

  // Act

  // Assert
  FAIL();
}

TEST(BraveAdsGetSignedTokensUrlRequestUtilTest,
     DoNotParseAndUnblindMissingSignedTokens) {
  // Arrange

  // Act

  // Assert
  FAIL();
}

TEST(BraveAdsGetSignedTokensUrlRequestUtilTest,
     DoNotParseAndUnblindInvalidSignedTokens) {
  // Arrange

  // Act

  // Assert
  FAIL();
}

TEST(BraveAdsGetSignedTokensUrlRequestUtilTest,
     DoNotParseAndUnblindSignedTokensIfMissingPublicKey) {
  // Arrange

  // Act

  // Assert
  FAIL();
}

TEST(BraveAdsGetSignedTokensUrlRequestUtilTest,
     DoNotVerifyAndUnblindInvalidSignedTokens) {
  // Arrange

  // Act

  // Assert
  FAIL();
}

TEST(BraveAdsGetSignedTokensUrlRequestUtilTest, BuildAndAddConfirmationTokens) {
  // Arrange

  // Act

  // Assert
  FAIL();
}

}  // namespace brave_ads
