/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/blinded_token_util.h"

#include <optional>

#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/blinded_token.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/token.h"

namespace brave_ads::cbr {

std::vector<BlindedToken> BlindTokens(const std::vector<Token>& tokens) {
  std::vector<BlindedToken> blinded_tokens;
  blinded_tokens.reserve(tokens.size());

  for (Token token : tokens) {
    if (!token.has_value()) {
      return {};
    }

    const std::optional<BlindedToken> blinded_token = token.Blind();
    if (!blinded_token) {
      return {};
    }

    blinded_tokens.push_back(*blinded_token);
  }

  return blinded_tokens;
}

std::vector<challenge_bypass_ristretto::BlindedToken> ToRawBlindedTokens(
    const std::vector<BlindedToken>& blinded_tokens) {
  std::vector<challenge_bypass_ristretto::BlindedToken> raw_blinded_tokens;
  raw_blinded_tokens.reserve(blinded_tokens.size());

  for (const auto& blinded_token : blinded_tokens) {
    if (!blinded_token.has_value()) {
      return {};
    }

    raw_blinded_tokens.push_back(blinded_token.get());
  }

  return raw_blinded_tokens;
}

}  // namespace brave_ads::cbr
