/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/privacy/challenge_bypass_ristretto/unblinded_token_util.h"

#include "bat/ads/internal/privacy/challenge_bypass_ristretto/unblinded_token.h"

namespace ads {
namespace privacy {
namespace cbr {

std::vector<UnblindedToken> ToUnblindedTokens(
    const std::vector<challenge_bypass_ristretto::UnblindedToken>& raw_tokens) {
  std::vector<UnblindedToken> unblinded_tokens;
  for (const auto& raw_token : raw_tokens) {
    UnblindedToken unblinded_token(raw_token);
    if (!unblinded_token.has_value()) {
      return {};
    }

    unblinded_tokens.push_back(unblinded_token);
  }

  return unblinded_tokens;
}

}  // namespace cbr
}  // namespace privacy
}  // namespace ads
