/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/utility/refill_confirmation_tokens/url_requests/get_signed_tokens/get_signed_tokens_url_request_util.h"

#include "base/test/values_test_util.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_tokens.h"
#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_tokens_test_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_tokens_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/token_generator_mock.h"
#include "brave/components/brave_ads/core/internal/account/wallet/wallet_info.h"
#include "brave/components/brave_ads/core/internal/account/wallet/wallet_test_util.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/public_key_test_util.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/unblinded_token_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsGetSignedTokensUrlRequestUtilTest : public test::TestBase {
 protected:
  TokenGeneratorMock token_generator_mock_;
};

TEST_F(BraveAdsGetSignedTokensUrlRequestUtilTest, ParseCaptchaId) {
  // Arrange
  const base::Value::Dict dict = base::test::ParseJsonDict(
      R"(
          {
            "captcha_id": "daf85dc8-164e-4eb9-a4d4-1836055004b3"
          })");

  // Act & Assert
  EXPECT_EQ("daf85dc8-164e-4eb9-a4d4-1836055004b3", ParseCaptchaId(dict));
}

TEST_F(BraveAdsGetSignedTokensUrlRequestUtilTest, DoNotParseMissingCaptchaId) {
  // Arrange
  const base::Value::Dict dict = base::test::ParseJsonDict("{}");

  // Act & Assert
  EXPECT_FALSE(ParseCaptchaId(dict));
}

TEST_F(BraveAdsGetSignedTokensUrlRequestUtilTest,
       BuildAndAddConfirmationTokens) {
  // Act
  BuildAndAddConfirmationTokens(cbr::test::GetUnblindedTokens(),
                                cbr::test::GetPublicKey(), test::Wallet());

  // Assert
  EXPECT_FALSE(GetConfirmationTokens().IsEmpty());
}

}  // namespace brave_ads
