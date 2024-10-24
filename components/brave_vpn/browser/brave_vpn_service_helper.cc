/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/brave_vpn_service_helper.h"

#include <algorithm>
#include <optional>
#include <utility>

#include "base/base64.h"
#include "base/json/values_util.h"
#include "base/notreached.h"
#include "base/ranges/algorithm.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/brave_vpn/common/brave_vpn_constants.h"
#include "brave/components/brave_vpn/common/brave_vpn_data_types.h"
#include "brave/components/brave_vpn/common/brave_vpn_utils.h"
#include "brave/components/brave_vpn/common/pref_names.h"
#include "brave/components/skus/browser/skus_utils.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace brave_vpn {
bool IsValidCredentialSummary(const base::Value& summary) {
  DCHECK(summary.is_dict());
  const bool active = summary.GetDict().FindBool("active").value_or(false);
  const int remaining_credential_count =
      summary.GetDict().FindInt("remaining_credential_count").value_or(0);
  return active && remaining_credential_count > 0;
}

bool IsValidCredentialSummaryButNeedActivation(const base::Value& summary) {
  DCHECK(summary.is_dict());
  const bool active = summary.GetDict().FindBool("active").value_or(false);
  const int remaining_credential_count =
      summary.GetDict().FindInt("remaining_credential_count").value_or(0);
  return !active && remaining_credential_count > 0;
}

bool HasSubscriberCredential(PrefService* local_prefs) {
  const base::Value::Dict& sub_cred_dict =
      local_prefs->GetDict(prefs::kBraveVPNSubscriberCredential);
  return !sub_cred_dict.empty();
}

std::optional<base::Time> GetExpirationTime(PrefService* local_prefs) {
  if (!HasValidSubscriberCredential(local_prefs))
    return std::nullopt;

  const base::Value::Dict& sub_cred_dict =
      local_prefs->GetDict(prefs::kBraveVPNSubscriberCredential);

  const base::Value* expiration_time_value =
      sub_cred_dict.Find(kSubscriberCredentialExpirationKey);

  if (!expiration_time_value)
    return std::nullopt;

  return base::ValueToTime(expiration_time_value);
}

void SetSubscriberCredential(PrefService* local_prefs,
                             const std::string& subscriber_credential,
                             const base::Time& expiration_time) {
  base::Value::Dict cred_dict;
  cred_dict.Set(kSubscriberCredentialKey, subscriber_credential);
  cred_dict.Set(kSubscriberCredentialExpirationKey,
                base::TimeToValue(expiration_time));
  local_prefs->SetDict(prefs::kBraveVPNSubscriberCredential,
                       std::move(cred_dict));
}

void ClearSubscriberCredential(PrefService* local_prefs) {
  local_prefs->ClearPref(prefs::kBraveVPNSubscriberCredential);
}

void SetSkusCredential(PrefService* local_prefs,
                       const std::string& skus_credential,
                       const base::Time& expiration_time) {
  base::Value::Dict cred_dict;
  cred_dict.Set(kSkusCredentialKey, skus_credential);
  cred_dict.Set(kSubscriberCredentialExpirationKey,
                base::TimeToValue(expiration_time));
  local_prefs->SetDict(prefs::kBraveVPNSubscriberCredential,
                       std::move(cred_dict));
  local_prefs->SetTime(prefs::kBraveVPNLastCredentialExpiry, expiration_time);
}

void SetSkusCredentialFetchingRetried(PrefService* local_prefs, bool retried) {
  ScopedDictPrefUpdate update(local_prefs,
                              prefs::kBraveVPNSubscriberCredential);
  update->Set(kRetriedSkusCredentialKey, base::Value(retried));
}

bool IsRetriedSkusCredential(PrefService* local_prefs) {
  const base::Value::Dict& sub_cred_dict =
      local_prefs->GetDict(prefs::kBraveVPNSubscriberCredential);
  return sub_cred_dict.FindBool(kRetriedSkusCredentialKey).value_or(false);
}

base::Time GetExpirationTimeForSkusCredential(PrefService* local_prefs) {
  CHECK(HasValidSkusCredential(local_prefs));

  const base::Value::Dict& sub_cred_dict =
      local_prefs->GetDict(prefs::kBraveVPNSubscriberCredential);

  const base::Value* expiration_time_value =
      sub_cred_dict.Find(kSubscriberCredentialExpirationKey);

  CHECK(expiration_time_value);
  return *base::ValueToTime(expiration_time_value);
}

}  // namespace brave_vpn
