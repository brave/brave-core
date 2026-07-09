/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/v2/credential_store.h"

#include <string>
#include <string_view>
#include <utility>

#include "base/check_deref.h"
#include "base/json/values_util.h"
#include "base/types/to_address.h"
#include "base/values.h"
#include "brave/components/brave_vpn/common/brave_vpn_constants.h"
#include "brave/components/brave_vpn/common/brave_vpn_utils.h"
#include "brave/components/brave_vpn/common/pref_names.h"
#include "components/prefs/pref_service.h"

namespace brave_vpn::v2 {
namespace {

// Shared validity check for a single keyed credential in the slot dictionary:
// the credential string must be present and non-empty, and the (shared)
// expiration must be present and in the future.
bool IsCredentialValid(const base::DictValue& dict,
                       std::string_view credential_key) {
  if (dict.empty()) {
    return false;
  }

  const std::string* credential = dict.FindString(credential_key);
  if (!credential || credential->empty()) {
    return false;
  }

  const base::Value* expiration_value =
      dict.Find(kSubscriberCredentialExpirationKey);
  if (!expiration_value) {
    return false;
  }

  const std::optional<base::Time> expiration =
      base::ValueToTime(expiration_value);
  return expiration.has_value() && *expiration >= base::Time::Now();
}

std::string GetCredential(const base::DictValue& dict,
                          std::string_view credential_key) {
  const std::string* credential = dict.FindString(credential_key);
  return credential ? *credential : std::string();
}

}  // namespace

CredentialStore::CredentialStore(PrefService* local_prefs)
    : local_prefs_(CHECK_DEREF(local_prefs)) {}

CredentialStore::~CredentialStore() = default;

bool CredentialStore::HasValidSubscriberCredential() const {
  return IsCredentialValid(
      local_prefs_->GetDict(prefs::kBraveVPNSubscriberCredential),
      kSubscriberCredentialKey);
}

std::string CredentialStore::GetSubscriberCredential() const {
  if (!HasValidSubscriberCredential()) {
    return std::string();
  }
  return GetCredential(
      local_prefs_->GetDict(prefs::kBraveVPNSubscriberCredential),
      kSubscriberCredentialKey);
}

void CredentialStore::SetSubscriberCredential(const std::string& credential,
                                              base::Time expiration) {
  base::DictValue dict;
  dict.Set(kSubscriberCredentialKey, credential);
  dict.Set(kSubscriberCredentialExpirationKey, base::TimeToValue(expiration));
  local_prefs_->SetDict(prefs::kBraveVPNSubscriberCredential, std::move(dict));
}

bool CredentialStore::HasValidSkusCredential() const {
  return IsCredentialValid(
      local_prefs_->GetDict(prefs::kBraveVPNSubscriberCredential),
      kSkusCredentialKey);
}

std::string CredentialStore::GetSkusCredential() const {
  if (!HasValidSkusCredential()) {
    return std::string();
  }
  return GetCredential(
      local_prefs_->GetDict(prefs::kBraveVPNSubscriberCredential),
      kSkusCredentialKey);
}

void CredentialStore::SetSkusCredential(const std::string& credential,
                                        base::Time expiration) {
  base::DictValue dict;
  dict.Set(kSkusCredentialKey, credential);
  dict.Set(kSubscriberCredentialExpirationKey, base::TimeToValue(expiration));
  local_prefs_->SetDict(prefs::kBraveVPNSubscriberCredential, std::move(dict));
  local_prefs_->SetTime(prefs::kBraveVPNLastCredentialExpiry, expiration);
}

bool CredentialStore::HasAnyCredential() const {
  return !local_prefs_->GetDict(prefs::kBraveVPNSubscriberCredential).empty();
}

std::optional<base::Time> CredentialStore::GetExpirationTime() const {
  if (!HasValidSubscriberCredential()) {
    return std::nullopt;
  }
  const base::DictValue& dict =
      local_prefs_->GetDict(prefs::kBraveVPNSubscriberCredential);
  const base::Value* expiration_value =
      dict.Find(kSubscriberCredentialExpirationKey);
  if (!expiration_value) {
    return std::nullopt;
  }
  return base::ValueToTime(expiration_value);
}

void CredentialStore::Clear() {
  ::brave_vpn::ClearSubscriberCredential(base::to_address(local_prefs_));
}

}  // namespace brave_vpn::v2
