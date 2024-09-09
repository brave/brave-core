/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_TOKENS_CONFIRMATION_TOKENS_CONFIRMATION_TOKENS_TEST_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_TOKENS_CONFIRMATION_TOKENS_CONFIRMATION_TOKENS_TEST_UTIL_H_

#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_token_info.h"

namespace brave_ads {

class ConfirmationTokens;

namespace test {

// Call this function to refill confirmation tokens for testing purposes if
// code paths call `Confirmations::Confirm`, `MaybeGetConfirmationToken`, or
// `BuildReward`. If code paths call `RefillConfirmationTokens::MaybeRefill`,
// call `MockTokenGenerator` instead and do not call this function.
ConfirmationTokenList RefillConfirmationTokens(int count);

ConfirmationTokenInfo BuildConfirmationToken();
ConfirmationTokenList BuildConfirmationTokens(int count);

}  // namespace test

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_TOKENS_CONFIRMATION_TOKENS_CONFIRMATION_TOKENS_TEST_UTIL_H_
