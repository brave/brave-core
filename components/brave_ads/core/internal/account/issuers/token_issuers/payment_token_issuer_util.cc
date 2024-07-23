/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/issuers/token_issuers/payment_token_issuer_util.h"

#include <optional>

#include "base/containers/flat_map.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_feature.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_info.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_util.h"
#include "brave/components/brave_ads/core/internal/account/issuers/token_issuers/token_issuer_info.h"
#include "brave/components/brave_ads/core/internal/account/issuers/token_issuers/token_issuer_types.h"
#include "brave/components/brave_ads/core/internal/account/issuers/token_issuers/token_issuer_util.h"

namespace brave_ads {

bool IsPaymentTokenIssuerValid(const IssuersInfo& issuers) {
  const std::optional<TokenIssuerInfo> payment_token_issuer =
      GetTokenIssuerForType(issuers, TokenIssuerType::kPayments);
  if (!payment_token_issuer) {
    return false;
  }

  const int maximum_token_issuer_public_keys =
      kMaximumTokenIssuerPublicKeys.Get();

  base::flat_map<double, int> buckets;
  for (const auto& [_, associated_value] : payment_token_issuer->public_keys) {
    ++buckets[associated_value];
    if (buckets[associated_value] > maximum_token_issuer_public_keys) {
      return false;
    }
  }

  return true;
}

}  // namespace brave_ads
