/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/skus/browser/skus_utils.h"

#include "base/command_line.h"
#include "base/notreached.h"
#include "brave/components/skus/browser/pref_names.h"
#include "brave/components/skus/browser/switches.h"
#include "components/pref_registry/pref_registry_syncable.h"

namespace skus {

// These domain prefixes are passed in as part of a full domain (see GetDomain)
// A domain with product is used by the following SKU methods:
// - `credential_summary`
// - `prepare_credentials_presentation`
constexpr char kProductTalk[] = "talk";
constexpr char kProductVPN[] = "vpn";

std::string GetEnvironment() {
  auto* cmd = base::CommandLine::ForCurrentProcess();
  if (!cmd->HasSwitch(switches::kSkusEnv)) {
#if defined(OFFICIAL_BUILD)
    return kEnvProduction;
#else
    return kEnvDevelopment;
#endif
  }

  const std::string value = cmd->GetSwitchValueASCII(switches::kSkusEnv);
  DCHECK(value == kEnvProduction || value == kEnvStaging ||
         value == kEnvDevelopment);
  return value;
}

std::string GetDomain(std::string prefix) {
  std::string environment = GetEnvironment();

  DCHECK(prefix == kProductTalk || prefix == kProductVPN);

  if (environment == kEnvProduction) {
    return prefix + ".brave.com";
  } else if (environment == kEnvStaging) {
    return prefix + ".bravesoftware.com";
  } else if (environment == kEnvDevelopment) {
    return prefix + ".brave.software";
  }

  NOTREACHED();

  return "";
}

void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterDictionaryPref(prefs::kSkusState);
}

}  // namespace skus
