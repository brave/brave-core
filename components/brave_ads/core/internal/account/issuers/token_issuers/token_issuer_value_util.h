/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_ISSUERS_TOKEN_ISSUERS_TOKEN_ISSUER_VALUE_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_ISSUERS_TOKEN_ISSUERS_TOKEN_ISSUER_VALUE_UTIL_H_

#include <optional>

#include "base/values.h"
#include "brave/components/brave_ads/core/internal/account/issuers/token_issuers/confirmation_token_issuer_info.h"
#include "brave/components/brave_ads/core/internal/account/issuers/token_issuers/payment_token_issuer_info.h"

namespace brave_ads {

struct TokenIssuersInfo final {
  ConfirmationTokenIssuerInfo confirmation;
  PaymentTokenIssuerInfo payment;
};

base::ListValue TokenIssuersToList(
    const ConfirmationTokenIssuerInfo& confirmation,
    const PaymentTokenIssuerInfo& payment);

std::optional<TokenIssuersInfo> MaybeBuildTokenIssuersFromList(
    const base::ListValue& list);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_ISSUERS_TOKEN_ISSUERS_TOKEN_ISSUER_VALUE_UTIL_H_
