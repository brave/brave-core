/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/privacy/tokens/token_generator.h"

#include "wrapper.hpp"

namespace ads {
namespace privacy {
namespace cbr {

using challenge_bypass_ristretto::Token;

TokenGenerator::TokenGenerator() = default;

TokenGenerator::~TokenGenerator() = default;

TokenList TokenGenerator::Generate(const int count) const {
  TokenList tokens;

  for (int i = 0; i < count; i++) {
    const Token& token = Token::random();
    tokens.push_back(token);
  }

  return tokens;
}

}  // namespace cbr
}  // namespace privacy
}  // namespace ads
