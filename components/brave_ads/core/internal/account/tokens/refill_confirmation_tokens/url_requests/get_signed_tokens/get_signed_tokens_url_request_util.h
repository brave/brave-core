/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_TOKENS_REFILL_CONFIRMATION_TOKENS_URL_REQUESTS_GET_SIGNED_TOKENS_GET_SIGNED_TOKENS_URL_REQUEST_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_TOKENS_REFILL_CONFIRMATION_TOKENS_URL_REQUESTS_GET_SIGNED_TOKENS_GET_SIGNED_TOKENS_URL_REQUEST_UTIL_H_

#include <string>
#include <tuple>
#include <vector>

#include "base/types/expected.h"
#include "base/values.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads {

namespace privacy::cbr {
class BlindedToken;
class PublicKey;
class Token;
class UnblindedToken;
}  // namespace privacy::cbr

struct WalletInfo;

absl::optional<std::string> ParseCaptchaId(const base::Value::Dict& dict);

base::expected<std::tuple<std::vector<privacy::cbr::UnblindedToken>,
                          privacy::cbr::PublicKey>,
               std::string>
ParseAndUnblindSignedTokens(
    const base::Value::Dict& dict,
    const std::vector<privacy::cbr::Token>& tokens,
    const std::vector<privacy::cbr::BlindedToken>& blinded_tokens);

void BuildAndAddConfirmationTokens(
    const std::vector<privacy::cbr::UnblindedToken>& unblinded_tokens,
    const privacy::cbr::PublicKey& public_key,
    const WalletInfo& wallet);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_TOKENS_REFILL_CONFIRMATION_TOKENS_URL_REQUESTS_GET_SIGNED_TOKENS_GET_SIGNED_TOKENS_URL_REQUEST_UTIL_H_
