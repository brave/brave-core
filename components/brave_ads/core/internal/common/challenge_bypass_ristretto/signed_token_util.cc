/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/signed_token_util.h"

#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/signed_token.h"

namespace brave_ads::cbr {

std::vector<challenge_bypass_ristretto::SignedToken> ToRawSignedTokens(
    const std::vector<SignedToken>& tokens) {
  std::vector<challenge_bypass_ristretto::SignedToken> raw_tokens;
  raw_tokens.reserve(tokens.size());

  for (const auto& token : tokens) {
    if (!token.has_value()) {
      return {};
    }

    raw_tokens.push_back(token.get());
  }

  return raw_tokens;
}

}  // namespace brave_ads::cbr
