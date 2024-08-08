/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_TOKENS_TOKEN_GENERATOR_TEST_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_TOKENS_TOKEN_GENERATOR_TEST_UTIL_H_

#include <cstddef>
#include <vector>

namespace brave_ads {

namespace cbr {
class Token;
}  // namespace cbr

namespace test {

// Use this function to mock the token generator for testing purposes, when code
// paths involve building reward confirmations.
void MockTokenGenerator(size_t count);

std::vector<cbr::Token> BuildTokens(size_t count);

}  // namespace test

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_TOKENS_TOKEN_GENERATOR_TEST_UTIL_H_
