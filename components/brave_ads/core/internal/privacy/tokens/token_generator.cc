/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/privacy/tokens/token_generator.h"

#include "base/check.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/token.h"

namespace brave_ads::privacy {

std::vector<cbr::Token> TokenGenerator::Generate(const size_t count) const {
  std::vector<cbr::Token> tokens;

  for (size_t i = 0; i < count; i++) {
    const cbr::Token token;
    CHECK(token.has_value());
    tokens.push_back(token);
  }

  return tokens;
}

}  // namespace brave_ads::privacy
