/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_TOKENS_TOKEN_GENERATOR_TEST_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_TOKENS_TOKEN_GENERATOR_TEST_UTIL_H_

#include <cstddef>

#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/token.h"

namespace brave_ads::test {

// Call this function to mock the token generator for testing purposes if code
// paths call `RefillConfirmationTokens::MaybeRefill`, `Confirmations::Confirm`,
// `BuildRewardConfirmation`, or `BuildReward`.
void MockTokenGenerator(size_t count);

cbr::TokenList BuildTokens(size_t count);

}  // namespace brave_ads::test

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_TOKENS_TOKEN_GENERATOR_TEST_UTIL_H_
