/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/tokens/test/fake_token_generator.h"

#include "base/check_op.h"

namespace brave_ads {

FakeTokenGenerator::FakeTokenGenerator() = default;

FakeTokenGenerator::~FakeTokenGenerator() = default;

void FakeTokenGenerator::SetTokens(cbr::TokenList tokens) {
  tokens_ = std::move(tokens);
}

cbr::TokenList FakeTokenGenerator::Generate(size_t count) const {
  CHECK_LE(count, tokens_.size());

  cbr::TokenList tokens(tokens_);
  tokens.resize(count);
  return tokens;
}

}  // namespace brave_ads
