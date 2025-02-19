/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/unblinded_token_test_util.h"

#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/challenge_bypass_ristretto_test_constants.h"

namespace brave_ads::cbr::test {

UnblindedToken GetUnblindedToken() {
  return UnblindedToken(kUnblindedTokenBase64);
}

UnblindedTokenList GetUnblindedTokens() {
  UnblindedTokenList unblinded_tokens;
  const UnblindedToken unblinded_token = GetUnblindedToken();
  unblinded_tokens.push_back(unblinded_token);
  return unblinded_tokens;
}

}  // namespace brave_ads::cbr::test
