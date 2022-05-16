/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/privacy/challenge_bypass_ristretto/signed_token_util.h"

#include "bat/ads/internal/privacy/challenge_bypass_ristretto/signed_token.h"

namespace ads {
namespace privacy {
namespace cbr {

std::vector<challenge_bypass_ristretto::SignedToken> ToRawSignedTokens(
    const std::vector<SignedToken>& tokens) {
  std::vector<challenge_bypass_ristretto::SignedToken> raw_tokens;
  for (const auto& token : tokens) {
    if (!token.has_value()) {
      return {};
    }

    raw_tokens.push_back(token.get());
  }

  return raw_tokens;
}

}  // namespace cbr
}  // namespace privacy
}  // namespace ads
