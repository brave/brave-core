/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BRAVE_VPN_SERVICE_HELPER_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BRAVE_VPN_SERVICE_HELPER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/values.h"
#include "brave/components/brave_vpn/mojom/brave_vpn.mojom.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

class PrefService;

namespace base {
class Time;
class Value;
}  // namespace base

namespace brave_vpn {
bool ValidateCachedRegionData(const base::Value::List& region_value);
mojom::RegionPtr GetRegionPtrWithNameFromRegionList(
    const std::string& name,
    const std::vector<mojom::Region> region_list);
std::vector<mojom::Region> ParseRegionList(
    const base::Value::List& region_list);
base::Value::Dict GetValueFromRegion(const mojom::Region& region);
bool IsValidCredentialSummary(const base::Value& summary);
bool HasSubscriberCredential(PrefService* local_prefs);
std::string GetSubscriberCredential(PrefService* local_prefs);
absl::optional<base::Time> GetExpirationTime(PrefService* local_prefs);
void SetSubscriberCredential(PrefService* local_prefs,
                             const std::string& subscriber_credential,
                             const base::Time& expiration_time);
void ClearSubscriberCredential(PrefService* local_prefs);

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BRAVE_VPN_SERVICE_HELPER_H_
