/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/privacy/challenge_bypass_ristretto/signed_token_unittest_util.h"

#include "bat/ads/internal/privacy/challenge_bypass_ristretto/challenge_bypass_ristretto_unittest_constants.h"

namespace ads {
namespace privacy {
namespace cbr {

SignedToken GetSignedToken() {
  return SignedToken(kSignedTokenBase64);
}

std::vector<SignedToken> GetSignedTokens() {
  std::vector<SignedToken> signed_tokens;
  const SignedToken signed_token = GetSignedToken();
  signed_tokens.push_back(signed_token);
  return signed_tokens;
}

SignedToken GetInvalidSignedToken() {
  return SignedToken(kInvalidBase64);
}

std::vector<SignedToken> GetInvalidSignedTokens() {
  std::vector<SignedToken> signed_tokens;
  const SignedToken signed_token = GetInvalidSignedToken();
  signed_tokens.push_back(signed_token);
  return signed_tokens;
}

}  // namespace cbr
}  // namespace privacy
}  // namespace ads
