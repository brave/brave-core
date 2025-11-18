/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/tokens/token_generator_test_util.h"

#include "base/check.h"
#include "base/check_op.h"
#include "brave/components/brave_ads/core/internal/account/tokens/token_generator_mock.h"
#include "brave/components/brave_ads/core/internal/ads_core/ads_core_util.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/token_test_util.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace brave_ads::test {

void MockTokenGenerator(size_t count) {
  const auto* const token_generator_mock =
      static_cast<const TokenGeneratorMock*>(GetTokenGenerator());
  ON_CALL(*token_generator_mock, Generate(count))
      .WillByDefault(::testing::Return(BuildTokens(count)));
}

cbr::TokenList BuildTokens(size_t count) {
  CHECK_GT(count, 0U);
  CHECK_LE(count, 50U);

  cbr::TokenList tokens;
  for (size_t i = 0; i < count; ++i) {
    const cbr::Token token = cbr::test::Tokens().at(i);
    CHECK(token.has_value());

    tokens.push_back(token);
  }

  return tokens;
}

}  // namespace brave_ads::test
