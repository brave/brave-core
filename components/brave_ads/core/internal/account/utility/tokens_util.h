/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_UTILITY_TOKENS_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_UTILITY_TOKENS_UTIL_H_

#include <optional>

#include "base/values.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/blinded_token.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/signed_token.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/token.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/unblinded_token.h"

namespace brave_ads {

namespace cbr {
class PublicKey;
}  // namespace cbr

std::optional<cbr::PublicKey> ParsePublicKey(const base::Value::Dict& dict);

std::optional<cbr::SignedTokenList> ParseSignedTokens(
    const base::Value::Dict& dict);

std::optional<cbr::UnblindedTokenList> ParseVerifyAndUnblindTokens(
    const base::Value::Dict& dict,
    const cbr::TokenList& tokens,
    const cbr::BlindedTokenList& blinded_tokens,
    const cbr::PublicKey& public_key);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_UTILITY_TOKENS_UTIL_H_
