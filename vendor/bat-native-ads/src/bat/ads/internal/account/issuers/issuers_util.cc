/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/issuers/issuers_util.h"

#include "base/ranges/algorithm.h"
#include "bat/ads/internal/account/issuers/confirmations_issuer_util.h"
#include "bat/ads/internal/account/issuers/issuer_info.h"
#include "bat/ads/internal/account/issuers/issuers_info.h"
#include "bat/ads/internal/account/issuers/issuers_value_util.h"
#include "bat/ads/internal/account/issuers/payments_issuer_util.h"
#include "bat/ads/internal/account/issuers/public_key_util.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "brave/components/brave_ads/common/pref_names.h"

namespace ads {

void SetIssuers(const IssuersInfo& issuers) {
  AdsClientHelper::GetInstance()->SetIntegerPref(prefs::kIssuerPing,
                                                 issuers.ping);

  AdsClientHelper::GetInstance()->SetListPref(prefs::kIssuers,
                                              IssuersToValue(issuers.issuers));
}

absl::optional<IssuersInfo> GetIssuers() {
  const absl::optional<base::Value::List> list =
      AdsClientHelper::GetInstance()->GetListPref(prefs::kIssuers);
  if (!list) {
    return absl::nullopt;
  }

  const absl::optional<IssuerList> issuer = ValueToIssuers(*list);
  if (!issuer) {
    return absl::nullopt;
  }

  IssuersInfo issuers;
  issuers.ping =
      AdsClientHelper::GetInstance()->GetIntegerPref(prefs::kIssuerPing);
  issuers.issuers = *issuer;

  return issuers;
}

void ResetIssuers() {
  AdsClientHelper::GetInstance()->ClearPref(prefs::kIssuerPing);
  AdsClientHelper::GetInstance()->ClearPref(prefs::kIssuers);
}

bool IsIssuersValid(const IssuersInfo& issuers) {
  return IsConfirmationsIssuerValid(issuers) && IsPaymentsIssuerValid(issuers);
}

bool HasIssuers() {
  return IssuerExistsForType(IssuerType::kConfirmations) &&
         IssuerExistsForType(IssuerType::kPayments);
}

bool HasIssuersChanged(const IssuersInfo& issuers) {
  const absl::optional<IssuersInfo> last_issuers = GetIssuers();
  if (!last_issuers) {
    return true;
  }

  return issuers != *last_issuers;
}

bool IssuerExistsForType(const IssuerType issuer_type) {
  const absl::optional<IssuersInfo> issuers = GetIssuers();
  if (!issuers) {
    return false;
  }

  const absl::optional<IssuerInfo> issuer =
      GetIssuerForType(*issuers, issuer_type);
  return static_cast<bool>(issuer);
}

absl::optional<IssuerInfo> GetIssuerForType(const IssuersInfo& issuers,
                                            const IssuerType issuer_type) {
  const auto iter =
      base::ranges::find(issuers.issuers, issuer_type, &IssuerInfo::type);
  if (iter == issuers.issuers.cend()) {
    return absl::nullopt;
  }

  return *iter;
}

bool PublicKeyExistsForIssuerType(const IssuerType issuer_type,
                                  const std::string& public_key) {
  const absl::optional<IssuersInfo> issuers = GetIssuers();
  if (!issuers) {
    return false;
  }

  const absl::optional<IssuerInfo> issuer =
      GetIssuerForType(*issuers, issuer_type);
  if (!issuer) {
    return false;
  }

  return PublicKeyExists(*issuer, public_key);
}

}  // namespace ads
