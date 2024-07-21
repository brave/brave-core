/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_ISSUERS_TOKEN_ISSUERS_TOKEN_ISSUER_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_ISSUERS_TOKEN_ISSUERS_TOKEN_ISSUER_UTIL_H_

#include <optional>

#include "brave/components/brave_ads/core/internal/account/issuers/token_issuers/token_issuer_types.h"

namespace brave_ads {

struct TokenIssuerInfo;
struct IssuersInfo;

namespace cbr {
class PublicKey;
}  // namespace cbr

bool TokenIssuerExistsForType(TokenIssuerType token_issuer_type);

std::optional<TokenIssuerInfo> GetTokenIssuerForType(
    const IssuersInfo& issuers,
    TokenIssuerType token_issuer_type);

bool TokenIssuerPublicKeyExistsForType(TokenIssuerType token_issuer_type,
                                       const cbr::PublicKey& public_key);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_ISSUERS_TOKEN_ISSUERS_TOKEN_ISSUER_UTIL_H_
