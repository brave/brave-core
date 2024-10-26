/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/issuers/issuers_util.h"

#include <string>

#include "base/ranges/algorithm.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_info.h"
#include "brave/components/brave_ads/core/internal/account/issuers/token_issuers/confirmation_token_issuer_util.h"
#include "brave/components/brave_ads/core/internal/account/issuers/token_issuers/payment_token_issuer_util.h"
#include "brave/components/brave_ads/core/internal/account/issuers/token_issuers/token_issuer_info.h"
#include "brave/components/brave_ads/core/internal/account/issuers/token_issuers/token_issuer_util.h"
#include "brave/components/brave_ads/core/internal/account/issuers/token_issuers/token_issuer_value_util.h"
#include "brave/components/brave_ads/core/internal/prefs/pref_util.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"

namespace brave_ads {

void SetIssuers(const IssuersInfo& issuers) {
  SetProfileIntegerPref(prefs::kIssuerPing, issuers.ping);

  SetProfileListPref(prefs::kIssuers,
                     TokenIssuersToValue(issuers.token_issuers));
}

std::optional<IssuersInfo> GetIssuers() {
  const std::optional<base::Value::List> list =
      GetProfileListPref(prefs::kIssuers);
  if (!list || list->empty()) {
    return std::nullopt;
  }

  const std::optional<TokenIssuerList> token_issuers =
      TokenIssuersFromValue(*list);
  if (!token_issuers) {
    return std::nullopt;
  }

  IssuersInfo issuers;
  issuers.ping = GetProfileIntegerPref(prefs::kIssuerPing);
  issuers.token_issuers = *token_issuers;

  return issuers;
}

bool IsIssuersValid(const IssuersInfo& issuers) {
  return IsConfirmationTokenIssuerValid(issuers) &&
         IsPaymentTokenIssuerValid(issuers);
}

bool HasIssuers() {
  return TokenIssuerExistsForType(TokenIssuerType::kConfirmations) &&
         TokenIssuerExistsForType(TokenIssuerType::kPayments);
}

bool HasIssuersChanged(const IssuersInfo& other) {
  const std::optional<IssuersInfo> issuers = GetIssuers();
  if (!issuers) {
    return true;
  }

  return other != *issuers;
}

}  // namespace brave_ads
