/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/privacy/challenge_bypass_ristretto/token_util.h"

#include "bat/ads/internal/privacy/challenge_bypass_ristretto/token.h"

namespace ads {
namespace privacy {
namespace cbr {

std::vector<challenge_bypass_ristretto::Token> ToRawTokens(
    const std::vector<Token>& tokens) {
  std::vector<challenge_bypass_ristretto::Token> raw_tokens;
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
