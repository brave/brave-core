/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/issuers/token_issuers/confirmation_token_issuer_util.h"

#include <cstddef>
#include <optional>

#include "brave/components/brave_ads/core/internal/account/issuers/issuers_feature.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_info.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_util.h"
#include "brave/components/brave_ads/core/internal/account/issuers/token_issuers/token_issuer_info.h"
#include "brave/components/brave_ads/core/internal/account/issuers/token_issuers/token_issuer_types.h"
#include "brave/components/brave_ads/core/internal/account/issuers/token_issuers/token_issuer_util.h"

namespace brave_ads {

bool IsConfirmationTokenIssuerValid(const IssuersInfo& issuers) {
  const std::optional<TokenIssuerInfo> token_issuer =
      GetTokenIssuerForType(issuers, TokenIssuerType::kConfirmations);
  if (!token_issuer) {
    return false;
  }

  return token_issuer->public_keys.size() <=
         static_cast<size_t>(kMaximumTokenIssuerPublicKeys.Get());
}

}  // namespace brave_ads
