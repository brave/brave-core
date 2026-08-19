/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/v2/credential_store.h"

#include <optional>
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

// Reads the keyed credential and the shared expiration from the slot as one
// bundle, or nullopt if the credential is not valid.
std::optional<CredentialStore::Credential> GetValidCredential(
    const base::DictValue& dict,
    std::string_view credential_key) {
  const std::string* credential = dict.FindString(credential_key);
  if (!credential || credential->empty()) {
    return std::nullopt;
  }

  const std::optional<base::Time> expiration =
      base::ValueToTime(dict.Find(kSubscriberCredentialExpirationKey));
  if (!expiration || *expiration < base::Time::Now()) {
    return std::nullopt;
  }

  return CredentialStore::Credential{
      .value = *credential,
      .expiration = *expiration,
  };
}

}  // namespace

CredentialStore::CredentialStore(PrefService* local_prefs)
    : local_prefs_(CHECK_DEREF(local_prefs)) {}

CredentialStore::~CredentialStore() = default;

std::optional<CredentialStore::Credential>
CredentialStore::GetValidSubscriberCredential() const {
  return GetValidCredential(
      local_prefs_->GetDict(prefs::kBraveVPNSubscriberCredential),
      kSubscriberCredentialKey);
}

void CredentialStore::SetSubscriberCredential(const Credential& credential) {
  local_prefs_->SetDict(prefs::kBraveVPNSubscriberCredential,
                        base::DictValue()
                            .Set(kSubscriberCredentialKey, credential.value)
                            .Set(kSubscriberCredentialExpirationKey,
                                 base::TimeToValue(credential.expiration)));
}

std::optional<CredentialStore::Credential>
CredentialStore::GetValidSkusCredential() const {
  return GetValidCredential(
      local_prefs_->GetDict(prefs::kBraveVPNSubscriberCredential),
      kSkusCredentialKey);
}

void CredentialStore::SetSkusCredential(const Credential& credential) {
  local_prefs_->SetDict(prefs::kBraveVPNSubscriberCredential,
                        base::DictValue()
                            .Set(kSkusCredentialKey, credential.value)
                            .Set(kSubscriberCredentialExpirationKey,
                                 base::TimeToValue(credential.expiration)));
  local_prefs_->SetTime(prefs::kBraveVPNLastCredentialExpiry,
                        credential.expiration);
}

bool CredentialStore::HasAnyCredential() const {
  return !local_prefs_->GetDict(prefs::kBraveVPNSubscriberCredential).empty();
}

void CredentialStore::Clear() {
  ::brave_vpn::ClearSubscriberCredential(base::to_address(local_prefs_));
}

}  // namespace brave_vpn::v2
