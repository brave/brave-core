/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/privacy/challenge_bypass_ristretto/blinded_token_util.h"

#include "absl/types/optional.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/blinded_token.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/token.h"

namespace ads::privacy::cbr {

std::vector<BlindedToken> BlindTokens(const std::vector<Token>& tokens) {
  std::vector<BlindedToken> blinded_tokens;
  for (Token token : tokens) {
    if (!token.has_value()) {
      return {};
    }

    const absl::optional<BlindedToken> blinded_token = token.Blind();
    if (!blinded_token) {
      return {};
    }

    blinded_tokens.push_back(*blinded_token);
  }

  return blinded_tokens;
}

std::vector<challenge_bypass_ristretto::BlindedToken> ToRawBlindedTokens(
    const std::vector<BlindedToken>& tokens) {
  std::vector<challenge_bypass_ristretto::BlindedToken> raw_tokens;
  for (const auto& token : tokens) {
    if (!token.has_value()) {
      return {};
    }

    raw_tokens.push_back(token.get());
  }

  return raw_tokens;
}

}  // namespace ads::privacy::cbr
