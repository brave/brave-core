/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/issuers/token_issuers/token_issuer_util.h"

#include <string>

#include "base/ranges/algorithm.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_info.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_util.h"
#include "brave/components/brave_ads/core/internal/account/issuers/token_issuers/confirmation_token_issuer_util.h"
#include "brave/components/brave_ads/core/internal/account/issuers/token_issuers/payment_token_issuer_util.h"
#include "brave/components/brave_ads/core/internal/account/issuers/token_issuers/token_issuer_info.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/public_key.h"

namespace brave_ads {

bool TokenIssuerExistsForType(const TokenIssuerType token_issuer_type) {
  const std::optional<IssuersInfo> issuers = GetIssuers();
  if (!issuers) {
    return false;
  }

  return !!GetTokenIssuerForType(*issuers, token_issuer_type);
}

std::optional<TokenIssuerInfo> GetTokenIssuerForType(
    const IssuersInfo& issuers,
    const TokenIssuerType token_issuer_type) {
  const auto iter = base::ranges::find(issuers.token_issuers, token_issuer_type,
                                       &TokenIssuerInfo::type);
  if (iter == issuers.token_issuers.cend()) {
    return std::nullopt;
  }

  return *iter;
}

bool TokenIssuerPublicKeyExistsForType(const TokenIssuerType token_issuer_type,
                                       const cbr::PublicKey& public_key) {
  const std::optional<IssuersInfo> issuers = GetIssuers();
  if (!issuers) {
    return false;
  }

  const std::optional<TokenIssuerInfo> token_issuer =
      GetTokenIssuerForType(*issuers, token_issuer_type);
  if (!token_issuer) {
    return false;
  }

  const std::optional<std::string> public_key_base64 =
      public_key.EncodeBase64();

  return public_key_base64 &&
         token_issuer->public_keys.contains(public_key_base64);
}

}  // namespace brave_ads
