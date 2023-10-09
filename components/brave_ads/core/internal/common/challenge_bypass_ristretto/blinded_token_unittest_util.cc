/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/blinded_token_unittest_util.h"

#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/blinded_token.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/challenge_bypass_ristretto_unittest_constants.h"

namespace brave_ads::cbr::test {

BlindedToken GetBlindedToken() {
  return BlindedToken(kBlindedTokenBase64);
}

BlindedToken GetInvalidBlindedToken() {
  return BlindedToken(kInvalidBase64);
}

std::vector<BlindedToken> GetBlindedTokens() {
  return {GetBlindedToken()};
}

std::vector<BlindedToken> GetInvalidBlindedTokens() {
  return {GetInvalidBlindedToken()};
}

}  // namespace brave_ads::cbr::test
