/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_UTILITY_REFILL_CONFIRMATION_TOKENS_URL_REQUESTS_GET_SIGNED_TOKENS_GET_SIGNED_TOKENS_URL_REQUEST_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_UTILITY_REFILL_CONFIRMATION_TOKENS_URL_REQUESTS_GET_SIGNED_TOKENS_GET_SIGNED_TOKENS_URL_REQUEST_UTIL_H_

#include <optional>
#include <string>

#include "base/values.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/unblinded_token.h"

namespace brave_ads {

namespace cbr {
class PublicKey;
}  // namespace cbr

struct WalletInfo;

std::optional<std::string> ParseCaptchaId(const base::Value::Dict& dict);

void BuildAndAddConfirmationTokens(
    const cbr::UnblindedTokenList& unblinded_tokens,
    const cbr::PublicKey& public_key,
    const WalletInfo& wallet);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_UTILITY_REFILL_CONFIRMATION_TOKENS_URL_REQUESTS_GET_SIGNED_TOKENS_GET_SIGNED_TOKENS_URL_REQUEST_UTIL_H_
