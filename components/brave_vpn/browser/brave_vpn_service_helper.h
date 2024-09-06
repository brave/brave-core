/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_BRAVE_VPN_SERVICE_HELPER_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_BRAVE_VPN_SERVICE_HELPER_H_

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/values.h"
#include "brave/components/brave_vpn/common/mojom/brave_vpn.mojom.h"

class PrefService;

namespace base {
class Time;
class Value;
}  // namespace base

namespace brave_vpn {
// False if subscription is expired.
bool IsValidCredentialSummary(const base::Value& summary);
bool IsValidCredentialSummaryButNeedActivation(const base::Value& summary);
bool HasSubscriberCredential(PrefService* local_prefs);
std::string GetSubscriberCredential(PrefService* local_prefs);
std::optional<base::Time> GetExpirationTime(PrefService* local_prefs);
void SetSubscriberCredential(PrefService* local_prefs,
                             const std::string& subscriber_credential,
                             const base::Time& expiration_time);
void ClearSubscriberCredential(PrefService* local_prefs);
void SetSkusCredential(PrefService* local_prefs,
                       const std::string& skus_credential,
                       const base::Time& expiration_time);
void SetSkusCredentialFetchingRetried(PrefService* local_prefs, bool retried);
bool IsRetriedSkusCredential(PrefService* local_prefs);
base::Time GetExpirationTimeForSkusCredential(PrefService* local_prefs);

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_BRAVE_VPN_SERVICE_HELPER_H_
