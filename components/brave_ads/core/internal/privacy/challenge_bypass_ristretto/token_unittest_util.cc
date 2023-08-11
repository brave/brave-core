/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/token_unittest_util.h"

#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/challenge_bypass_ristretto_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/token.h"

namespace brave_ads::privacy::cbr {

Token GetTokenForTesting() {
  return Token(kTokenBase64);
}

std::vector<Token> GetTokensForTesting() {
  return {GetTokenForTesting()};
}

Token GetInvalidTokenForTesting() {
  return Token(kInvalidBase64);
}

std::vector<Token> GetInvalidTokensForTesting() {
  return {GetInvalidTokenForTesting()};
}

}  // namespace brave_ads::privacy::cbr
