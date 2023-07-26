/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PRIVACY_TOKENS_CONFIRMATION_TOKENS_CONFIRMATION_TOKENS_UNITTEST_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PRIVACY_TOKENS_CONFIRMATION_TOKENS_CONFIRMATION_TOKENS_UNITTEST_UTIL_H_

#include <string>
#include <vector>

#include "brave/components/brave_ads/core/internal/privacy/tokens/confirmation_tokens/confirmation_token_info.h"

namespace brave_ads {

struct WalletInfo;

namespace privacy {

class ConfirmationTokens;

ConfirmationTokens& GetConfirmationTokens();

ConfirmationTokenList SetConfirmationTokens(int count);

ConfirmationTokenList BuildConfirmationTokens(
    const std::vector<std::string>& unblinded_tokens_base64,
    const WalletInfo& wallet);
ConfirmationTokenInfo BuildConfirmationToken(
    const std::string& unblinded_token_base64,
    const WalletInfo& wallet);

ConfirmationTokenList BuildConfirmationTokens(int count);
ConfirmationTokenInfo BuildConfirmationToken();

}  // namespace privacy
}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PRIVACY_TOKENS_CONFIRMATION_TOKENS_CONFIRMATION_TOKENS_UNITTEST_UTIL_H_
