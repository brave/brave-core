/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PRIVACY_TOKENS_TOKEN_GENERATOR_UNITTEST_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PRIVACY_TOKENS_TOKEN_GENERATOR_UNITTEST_UTIL_H_

#include <vector>

#include "brave/components/brave_ads/core/internal/privacy/tokens/token_generator_mock.h"

namespace brave_ads::privacy {

namespace cbr {
class Token;
}  // namespace cbr

std::vector<cbr::Token> BuildTokens(size_t count);

void MockTokenGenerator(const TokenGeneratorMock& mock, size_t count);

}  // namespace brave_ads::privacy

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PRIVACY_TOKENS_TOKEN_GENERATOR_UNITTEST_UTIL_H_
