/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_TOKENS_TOKEN_GENERATOR_MOCK_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_TOKENS_TOKEN_GENERATOR_MOCK_H_

#include <cstddef>
#include <vector>

#include "brave/components/brave_ads/core/internal/account/tokens/token_generator.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/token.h"
#include "testing/gmock/include/gmock/gmock.h"  // IWYU pragma: keep

namespace brave_ads {

class TokenGeneratorMock : public TokenGenerator {
 public:
  TokenGeneratorMock();

  TokenGeneratorMock(const TokenGeneratorMock&) = delete;
  TokenGeneratorMock& operator=(const TokenGeneratorMock&) = delete;

  TokenGeneratorMock(TokenGeneratorMock&&) noexcept = delete;
  TokenGeneratorMock& operator=(TokenGeneratorMock&&) noexcept = delete;

  ~TokenGeneratorMock() override;

  MOCK_METHOD(std::vector<cbr::Token>, Generate, (const size_t count), (const));
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_TOKENS_TOKEN_GENERATOR_MOCK_H_
