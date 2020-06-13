/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/confirmations/internal/privacy_utils.h"

#include "base/logging.h"

namespace confirmations {
namespace privacy {

std::vector<Token> GenerateTokens(
    const int count) {
  DCHECK_GT(count, 0);

  std::vector<Token> tokens;

  for (int i = 0; i < count; i++) {
    auto token = Token::random();
    tokens.push_back(token);
  }

  return tokens;
}

std::vector<BlindedToken> BlindTokens(
    const std::vector<Token>& tokens) {
  DCHECK_NE(tokens.size(), 0UL);

  std::vector<BlindedToken> blinded_tokens;
  for (unsigned int i = 0; i < tokens.size(); i++) {
    auto token = tokens.at(i);
    auto blinded_token = token.blind();

    blinded_tokens.push_back(blinded_token);
  }

  return blinded_tokens;
}

}  // namespace privacy
}  // namespace confirmations
