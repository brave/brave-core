/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/issuers/issuers_util.h"

#include <algorithm>

#include "base/check.h"
#include "base/containers/flat_map.h"
#include "bat/ads/internal/account/issuers/issuer_info.h"
#include "bat/ads/internal/account/issuers/issuers_info.h"
#include "bat/ads/internal/account/issuers/issuers_value_util.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/pref_names.h"

namespace ads {

namespace {

constexpr int kMaximumIssuerPublicKeys = 3;

bool PublicKeyExists(const IssuerInfo& issuer, const std::string& public_key) {
  const auto iter = issuer.public_keys.find(public_key);
  return iter != issuer.public_keys.cend();
}

}  // namespace

bool IsIssuerValid(const IssuerInfo& issuer) {
  base::flat_map<double, int> buckets;
  for (const auto& public_key : issuer.public_keys) {
    const double bucket = public_key.second;
    buckets[bucket]++;

    const int count = buckets[bucket];
    if (count > kMaximumIssuerPublicKeys) {
      return false;
    }
  }

  return true;
}

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

bool HasIssuers() {
  return !(!IssuerExistsForType(IssuerType::kConfirmations) ||
           !IssuerExistsForType(IssuerType::kPayments));
}

bool HasIssuersChanged(const IssuersInfo& issuers) {
  const absl::optional<IssuersInfo> last_issuers = GetIssuers();
  if (!last_issuers) {
    return false;
  }

  if (issuers == *last_issuers) {
    return false;
  }

  return true;
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
      std::find_if(issuers.issuers.cbegin(), issuers.issuers.cend(),
                   [issuer_type](const IssuerInfo& issuer) {
                     return issuer.type == issuer_type;
                   });
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
