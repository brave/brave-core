/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_COMMON_BRAVE_VPN_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_COMMON_BRAVE_VPN_UTILS_H_

#include <string>

#include "build/build_config.h"

class PrefRegistrySimple;
class PrefService;
namespace user_prefs {
class PrefRegistrySyncable;
}  // namespace user_prefs

namespace brave_vpn {

bool IsBraveVPNEnabled();
std::string GetBraveVPNPaymentsEnv(const std::string& env);
std::string GetManageUrl(const std::string& env);
void MigrateVPNSettings(PrefService* profile_prefs, PrefService* local_prefs);
void RegisterLocalStatePrefs(PrefRegistrySimple* registry);
void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);
void RegisterAndroidProfilePrefs(PrefRegistrySimple* registry);
bool HasValidSubscriberCredential(PrefService* local_prefs);
std::string GetSubscriberCredential(PrefService* local_prefs);

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_COMMON_BRAVE_VPN_UTILS_H_
