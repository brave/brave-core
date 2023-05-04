/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/blinded_token_unittest_util.h"

#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/blinded_token.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/challenge_bypass_ristretto_unittest_constants.h"

namespace brave_ads::privacy::cbr {

BlindedToken GetBlindedToken() {
  return BlindedToken(kBlindedTokenBase64);
}

std::vector<BlindedToken> GetBlindedTokens() {
  return {GetBlindedToken()};
}

BlindedToken GetInvalidBlindedToken() {
  return BlindedToken(kInvalidBase64);
}

std::vector<BlindedToken> GetInvalidBlindedTokens() {
  return {GetInvalidBlindedToken()};
}

}  // namespace brave_ads::privacy::cbr
