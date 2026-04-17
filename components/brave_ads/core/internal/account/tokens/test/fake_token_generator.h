/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_TOKENS_TEST_FAKE_TOKEN_GENERATOR_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_TOKENS_TEST_FAKE_TOKEN_GENERATOR_H_

#include <cstddef>

#include "brave/components/brave_ads/core/internal/account/tokens/token_generator_interface.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/token.h"

namespace brave_ads {

// A test double for `TokenGeneratorInterface` that returns a fixed list of
// tokens. Call `SetTokens` to configure the tokens returned by `Generate`.
class FakeTokenGenerator : public TokenGeneratorInterface {
 public:
  FakeTokenGenerator();

  FakeTokenGenerator(const FakeTokenGenerator&) = delete;
  FakeTokenGenerator& operator=(const FakeTokenGenerator&) = delete;

  ~FakeTokenGenerator() override;

  void SetTokens(cbr::TokenList tokens);

  // TokenGeneratorInterface:
  cbr::TokenList Generate(size_t count) const override;

 private:
  cbr::TokenList tokens_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_TOKENS_TEST_FAKE_TOKEN_GENERATOR_H_
