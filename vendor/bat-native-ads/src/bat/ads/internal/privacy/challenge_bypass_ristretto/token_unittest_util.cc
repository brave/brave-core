/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/privacy/challenge_bypass_ristretto/token_unittest_util.h"

#include "bat/ads/internal/privacy/challenge_bypass_ristretto/challenge_bypass_ristretto_unittest_constants.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/token.h"

namespace ads::privacy::cbr {

Token GetToken() {
  return Token(kTokenBase64);
}

std::vector<Token> GetTokens() {
  std::vector<Token> tokens;
  const Token token = GetToken();
  tokens.push_back(token);
  return tokens;
}

Token GetInvalidToken() {
  return Token(kInvalidBase64);
}

std::vector<Token> GetInvalidTokens() {
  std::vector<Token> tokens;
  const Token token = GetInvalidToken();
  tokens.push_back(token);
  return tokens;
}

}  // namespace ads::privacy::cbr
