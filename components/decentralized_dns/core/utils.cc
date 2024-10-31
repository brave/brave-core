/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/decentralized_dns/core/utils.h"

#include <string_view>

#include "base/strings/string_util.h"
#include "brave/components/decentralized_dns/core/constants.h"
#include "brave/components/decentralized_dns/core/pref_names.h"
#include "brave/net/decentralized_dns/constants.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace decentralized_dns {

void RegisterLocalStatePrefs(PrefRegistrySimple* registry) {
  registry->RegisterIntegerPref(kUnstoppableDomainsResolveMethod,
                                static_cast<int>(ResolveMethodTypes::ASK));
  registry->RegisterIntegerPref(kENSResolveMethod,
                                static_cast<int>(ResolveMethodTypes::ASK));
  registry->RegisterIntegerPref(
      kEnsOffchainResolveMethod,
      static_cast<int>(EnsOffchainResolveMethod::kAsk));
  registry->RegisterIntegerPref(kSnsResolveMethod,
                                static_cast<int>(ResolveMethodTypes::ASK));

  // Register prefs for migration.
  // Added 12/2023 to reset SNS pref to re-opt in with updated interstitial.
  registry->RegisterBooleanPref(kSnsResolveMethodMigrated, false);
}

void MigrateObsoleteLocalStatePrefs(PrefService* local_state) {
  // Added 05/2022
  if (static_cast<int>(ResolveMethodTypes::DEPRECATED_DNS_OVER_HTTPS) ==
      local_state->GetInteger(kUnstoppableDomainsResolveMethod)) {
    local_state->ClearPref(kUnstoppableDomainsResolveMethod);
  }
  if (static_cast<int>(ResolveMethodTypes::DEPRECATED_DNS_OVER_HTTPS) ==
      local_state->GetInteger(kENSResolveMethod)) {
    local_state->ClearPref(kENSResolveMethod);
  }

  // Added 12/2023
  // Reset SNS resolve method to ask to re-opt in with updated interstitial.
  if (!local_state->GetBoolean(kSnsResolveMethodMigrated)) {
    if (local_state->GetInteger(kSnsResolveMethod) ==
        static_cast<int>(ResolveMethodTypes::ENABLED)) {
      local_state->ClearPref(kSnsResolveMethod);
    }
    local_state->SetBoolean(kSnsResolveMethodMigrated, true);
  }
}

bool IsUnstoppableDomainsTLD(std::string_view host) {
  for (auto* domain : kUnstoppableDomains) {
    if (host.ends_with(domain)) {
      return true;
    }
  }
  return false;
}

void SetUnstoppableDomainsResolveMethod(PrefService* local_state,
                                        ResolveMethodTypes method) {
  local_state->SetInteger(kUnstoppableDomainsResolveMethod,
                          static_cast<int>(method));
}

ResolveMethodTypes GetUnstoppableDomainsResolveMethod(
    PrefService* local_state) {
  return static_cast<ResolveMethodTypes>(
      local_state->GetInteger(kUnstoppableDomainsResolveMethod));
}

bool IsUnstoppableDomainsResolveMethodAsk(PrefService* local_state) {
  if (!local_state) {
    return false;  // Treat it as disabled.
  }

  return GetUnstoppableDomainsResolveMethod(local_state) ==
         ResolveMethodTypes::ASK;
}

bool IsUnstoppableDomainsResolveMethodEnabled(PrefService* local_state) {
  if (!local_state) {
    return false;  // Treat it as disabled.
  }

  return GetUnstoppableDomainsResolveMethod(local_state) ==
         ResolveMethodTypes::ENABLED;
}

bool IsENSTLD(std::string_view host) {
  return host.ends_with(kEthDomain);
}

void SetENSResolveMethod(PrefService* local_state, ResolveMethodTypes method) {
  local_state->SetInteger(kENSResolveMethod, static_cast<int>(method));
}

ResolveMethodTypes GetENSResolveMethod(PrefService* local_state) {
  return static_cast<ResolveMethodTypes>(
      local_state->GetInteger(kENSResolveMethod));
}

bool IsENSResolveMethodAsk(PrefService* local_state) {
  if (!local_state) {
    return false;  // Treat it as disabled.
  }

  return GetENSResolveMethod(local_state) == ResolveMethodTypes::ASK;
}

bool IsENSResolveMethodEnabled(PrefService* local_state) {
  if (!local_state) {
    return false;  // Treat it as disabled.
  }

  return GetENSResolveMethod(local_state) == ResolveMethodTypes::ENABLED;
}

void SetEnsOffchainResolveMethod(PrefService* local_state,
                                 EnsOffchainResolveMethod method) {
  local_state->SetInteger(kEnsOffchainResolveMethod, static_cast<int>(method));
}

EnsOffchainResolveMethod GetEnsOffchainResolveMethod(PrefService* local_state) {
  return static_cast<EnsOffchainResolveMethod>(
      local_state->GetInteger(kEnsOffchainResolveMethod));
}

bool IsSnsTLD(std::string_view host) {
  return host.ends_with(kSolDomain);
}

void SetSnsResolveMethod(PrefService* local_state, ResolveMethodTypes method) {
  local_state->SetInteger(kSnsResolveMethod, static_cast<int>(method));
}

ResolveMethodTypes GetSnsResolveMethod(PrefService* local_state) {
  return static_cast<ResolveMethodTypes>(
      local_state->GetInteger(kSnsResolveMethod));
}

bool IsSnsResolveMethodAsk(PrefService* local_state) {
  if (!local_state) {
    return false;  // Treat it as disabled.
  }

  return GetSnsResolveMethod(local_state) == ResolveMethodTypes::ASK;
}

bool IsSnsResolveMethodEnabled(PrefService* local_state) {
  if (!local_state) {
    return false;  // Treat it as disabled.
  }

  return GetSnsResolveMethod(local_state) == ResolveMethodTypes::ENABLED;
}

}  // namespace decentralized_dns
