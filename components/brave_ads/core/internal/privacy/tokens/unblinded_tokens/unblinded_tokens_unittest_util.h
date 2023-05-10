/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PRIVACY_TOKENS_UNBLINDED_TOKENS_UNBLINDED_TOKENS_UNITTEST_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PRIVACY_TOKENS_UNBLINDED_TOKENS_UNBLINDED_TOKENS_UNITTEST_UTIL_H_

#include <string>
#include <vector>

#include "brave/components/brave_ads/core/internal/privacy/tokens/unblinded_tokens/unblinded_token_info.h"

namespace brave_ads {

struct WalletInfo;

namespace privacy {

class UnblindedTokens;

UnblindedTokens& GetUnblindedTokens();

UnblindedTokenList SetUnblindedTokens(int count);

UnblindedTokenInfo CreateUnblindedToken(
    const std::string& unblinded_token_base64,
    const WalletInfo& wallet);
UnblindedTokenList CreateUnblindedTokens(
    const std::vector<std::string>& unblinded_tokens_base64,
    const WalletInfo& wallet);

UnblindedTokenList BuildUnblindedTokens(int count);
UnblindedTokenInfo BuildUnblindedToken();

}  // namespace privacy
}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PRIVACY_TOKENS_UNBLINDED_TOKENS_UNBLINDED_TOKENS_UNITTEST_UTIL_H_
