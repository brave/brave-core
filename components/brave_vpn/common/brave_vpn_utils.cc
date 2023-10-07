/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/common/brave_vpn_utils.h"

#include <utility>

#include "base/feature_list.h"
#include "base/json/json_writer.h"
#include "base/json/values_util.h"
#include "base/notreached.h"
#include "base/strings/string_split.h"
#include "brave/components/brave_vpn/common/brave_vpn_constants.h"
#include "brave/components/brave_vpn/common/features.h"
#include "brave/components/brave_vpn/common/pref_names.h"
#include "brave/components/p3a_utils/feature_usage.h"
#include "brave/components/skus/browser/skus_utils.h"
#include "brave/components/skus/common/features.h"
#include "build/build_config.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/version_info/channel.h"

#if BUILDFLAG(IS_WIN)
#include "brave/components/brave_vpn/common/wireguard/win/wireguard_utils_win.h"
#endif

namespace brave_vpn {

namespace {
void RegisterVPNLocalStatePrefs(PrefRegistrySimple* registry) {
#if !BUILDFLAG(IS_ANDROID)
  registry->RegisterListPref(prefs::kBraveVPNRegionList);
  registry->RegisterTimePref(prefs::kBraveVPNRegionListFetchedDate, {});
  registry->RegisterStringPref(prefs::kBraveVPNDeviceRegion, "");
  registry->RegisterStringPref(prefs::kBraveVPNSelectedRegion, "");
#endif
  registry->RegisterStringPref(prefs::kBraveVPNEnvironment,
                               skus::GetDefaultEnvironment());
  registry->RegisterStringPref(prefs::kBraveVPNWireguardProfileCredentials, "");
  registry->RegisterDictionaryPref(prefs::kBraveVPNRootPref);
  registry->RegisterDictionaryPref(prefs::kBraveVPNSubscriberCredential);
  registry->RegisterBooleanPref(prefs::kBraveVPNLocalStateMigrated, false);
  registry->RegisterTimePref(prefs::kBraveVPNSessionExpiredDate, {});
#if BUILDFLAG(ENABLE_BRAVE_VPN_WIREGUARD)
  registry->RegisterBooleanPref(prefs::kBraveVPNWireguardEnabled, false);
#endif
}

}  // namespace

bool IsBraveVPNWireguardEnabled(PrefService* local_state) {
  DCHECK(IsBraveVPNFeatureEnabled());
#if BUILDFLAG(ENABLE_BRAVE_VPN_WIREGUARD)
  return local_state->GetBoolean(prefs::kBraveVPNWireguardEnabled);
#else
  return false;
#endif
}
#if BUILDFLAG(IS_WIN)
void MigrateWireguardFeatureFlag(PrefService* local_prefs) {
  auto* wireguard_enabled_pref =
      local_prefs->FindPreference(prefs::kBraveVPNWireguardEnabled);
  if (wireguard_enabled_pref && wireguard_enabled_pref->IsDefaultValue()) {
    local_prefs->SetBoolean(
        prefs::kBraveVPNWireguardEnabled,
        base::FeatureList::IsEnabled(features::kBraveVPNUseWireguardService) &&
            brave_vpn::wireguard::IsWireguardServiceRegistered());
  }
}
#endif  // BUILDFLAG(IS_WIN)
void MigrateVPNSettings(PrefService* profile_prefs, PrefService* local_prefs) {
  if (local_prefs->GetBoolean(prefs::kBraveVPNLocalStateMigrated)) {
    return;
  }

  if (!profile_prefs->HasPrefPath(prefs::kBraveVPNRootPref)) {
    local_prefs->SetBoolean(prefs::kBraveVPNLocalStateMigrated, true);
    return;
  }
  base::Value::Dict obsolete_pref =
      profile_prefs->GetDict(prefs::kBraveVPNRootPref).Clone();
  base::Value::Dict result;
  if (local_prefs->HasPrefPath(prefs::kBraveVPNRootPref)) {
    result = local_prefs->GetDict(prefs::kBraveVPNRootPref).Clone();
    auto& result_dict = result;
    result_dict.Merge(std::move(obsolete_pref));
  } else {
    result = std::move(obsolete_pref);
  }
  // Do not migrate brave_vpn::prefs::kBraveVPNShowButton, we want it to be
  // inside the profile preferences.
  auto tokens =
      base::SplitString(brave_vpn::prefs::kBraveVPNShowButton, ".",
                        base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  if (result.FindBool(tokens.back())) {
    result.Remove(tokens.back());
  }
  local_prefs->Set(prefs::kBraveVPNRootPref, base::Value(std::move(result)));
  local_prefs->SetBoolean(prefs::kBraveVPNLocalStateMigrated, true);

  bool show_button =
      profile_prefs->GetBoolean(brave_vpn::prefs::kBraveVPNShowButton);
  profile_prefs->ClearPref(prefs::kBraveVPNRootPref);
  // Set kBraveVPNShowButton back, it is only one per profile preference for
  // now.
  profile_prefs->SetBoolean(brave_vpn::prefs::kBraveVPNShowButton, show_button);
}

bool IsBraveVPNDisabledByPolicy(PrefService* prefs) {
  DCHECK(prefs);
  return prefs->FindPreference(prefs::kManagedBraveVPNDisabled) &&
  // Need to investigate more about this.
  // IsManagedPreference() gives false on macOS when it's configured by
  // "defaults write com.brave.Browser.beta BraveVPNDisabled -bool true".
  // As kManagedBraveVPNDisabled is false by default and only can be set
  // by policy, I think skipping this condition checking will be fine.
#if !BUILDFLAG(IS_MAC)
         prefs->IsManagedPreference(prefs::kManagedBraveVPNDisabled) &&
#endif
         prefs->GetBoolean(prefs::kManagedBraveVPNDisabled);
}

bool IsBraveVPNFeatureEnabled() {
  return base::FeatureList::IsEnabled(brave_vpn::features::kBraveVPN) &&
         base::FeatureList::IsEnabled(skus::features::kSkusFeature);
}

bool IsBraveVPNEnabled(PrefService* prefs) {
  return !IsBraveVPNDisabledByPolicy(prefs) && IsBraveVPNFeatureEnabled();
}

std::string GetBraveVPNEntryName(version_info::Channel channel) {
  constexpr char kBraveVPNEntryName[] = "BraveVPN";

  const std::string entry_name(kBraveVPNEntryName);
  switch (channel) {
    case version_info::Channel::UNKNOWN:
      return entry_name + "Development";
    case version_info::Channel::CANARY:
      return entry_name + "Nightly";
    case version_info::Channel::DEV:
      return entry_name + "Dev";
    case version_info::Channel::BETA:
      return entry_name + "Beta";
    case version_info::Channel::STABLE:
      return entry_name;
    default:
      return entry_name;
  }
}

std::string GetManageUrl(const std::string& env) {
  if (env == skus::kEnvProduction)
    return brave_vpn::kManageUrlProd;
  if (env == skus::kEnvStaging)
    return brave_vpn::kManageUrlStaging;
  if (env == skus::kEnvDevelopment)
    return brave_vpn::kManageUrlDev;

  NOTREACHED();
  return brave_vpn::kManageUrlProd;
}

// On desktop, the environment is tied to SKUs because you would purchase it
// from `account.brave.com` (or similar, based on env). The credentials for VPN
// will always be in the same environment as the SKU environment.
//
// When the vendor receives a credential from us during auth, it also includes
// the environment. The vendor then can do a lookup using Payment Service.
std::string GetBraveVPNPaymentsEnv(const std::string& env) {
  // Use same string as payment env.
  return env;
}

void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterBooleanPref(prefs::kManagedBraveVPNDisabled, false);
  registry->RegisterDictionaryPref(prefs::kBraveVPNRootPref);
  registry->RegisterBooleanPref(prefs::kBraveVPNShowButton, true);
#if BUILDFLAG(IS_WIN)
  registry->RegisterBooleanPref(prefs::kBraveVPNShowNotificationDialog, true);
  registry->RegisterBooleanPref(prefs::kBraveVPNWireguardFallbackDialog, true);
#endif
#if BUILDFLAG(IS_ANDROID)
  registry->RegisterStringPref(prefs::kBraveVPNPurchaseTokenAndroid, "");
  registry->RegisterStringPref(prefs::kBraveVPNPackageAndroid, "");
  registry->RegisterStringPref(prefs::kBraveVPNProductIdAndroid, "");
#endif
}

void RegisterLocalStatePrefs(PrefRegistrySimple* registry) {
  p3a_utils::RegisterFeatureUsagePrefs(
      registry, prefs::kBraveVPNFirstUseTime, prefs::kBraveVPNLastUseTime,
      prefs::kBraveVPNUsedSecondDay, prefs::kBraveVPNDaysInMonthUsed, nullptr);
  RegisterVPNLocalStatePrefs(registry);
}

bool HasValidSubscriberCredential(PrefService* local_prefs) {
  const base::Value::Dict& sub_cred_dict =
      local_prefs->GetDict(prefs::kBraveVPNSubscriberCredential);
  if (sub_cred_dict.empty())
    return false;

  const std::string* cred = sub_cred_dict.FindString(kSubscriberCredentialKey);
  const base::Value* expiration_time_value =
      sub_cred_dict.Find(kSubscriberCredentialExpirationKey);

  if (!cred || !expiration_time_value)
    return false;

  if (cred->empty())
    return false;

  auto expiration_time = base::ValueToTime(expiration_time_value);
  if (!expiration_time || expiration_time < base::Time::Now())
    return false;

  return true;
}

std::string GetSubscriberCredential(PrefService* local_prefs) {
  if (!HasValidSubscriberCredential(local_prefs))
    return "";
  const base::Value::Dict& sub_cred_dict =
      local_prefs->GetDict(prefs::kBraveVPNSubscriberCredential);
  const std::string* cred = sub_cred_dict.FindString(kSubscriberCredentialKey);
  DCHECK(cred);
  return *cred;
}

bool HasValidSkusCredential(PrefService* local_prefs) {
  const base::Value::Dict& sub_cred_dict =
      local_prefs->GetDict(prefs::kBraveVPNSubscriberCredential);
  if (sub_cred_dict.empty()) {
    return false;
  }

  const std::string* skus_cred = sub_cred_dict.FindString(kSkusCredentialKey);
  const base::Value* expiration_time_value =
      sub_cred_dict.Find(kSubscriberCredentialExpirationKey);

  if (!skus_cred || !expiration_time_value) {
    return false;
  }

  if (skus_cred->empty()) {
    return false;
  }

  auto expiration_time = base::ValueToTime(expiration_time_value);
  if (!expiration_time || expiration_time < base::Time::Now()) {
    return false;
  }

  return true;
}

std::string GetSkusCredential(PrefService* local_prefs) {
  CHECK(HasValidSkusCredential(local_prefs))
      << "Don't call when there is no valid skus credential.";

  const base::Value::Dict& sub_cred_dict =
      local_prefs->GetDict(prefs::kBraveVPNSubscriberCredential);
  const std::string* skus_cred = sub_cred_dict.FindString(kSkusCredentialKey);
  DCHECK(skus_cred);
  return *skus_cred;
}

}  // namespace brave_vpn
