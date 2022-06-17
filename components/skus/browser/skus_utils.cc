/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/skus/browser/skus_utils.h"

#include "base/command_line.h"
#include "base/notreached.h"
#include "base/strings/string_util.h"
#include "brave/components/skus/browser/pref_names.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"

namespace skus {

// These domain prefixes are passed in as part of a full domain (see GetDomain)
// A domain with product is used by the following SKU methods:
// - `credential_summary`
// - `prepare_credentials_presentation`
constexpr char kProductTalk[] = "talk";
constexpr char kProductVPN[] = "vpn";

std::string GetDefaultEnvironment() {
#if defined(OFFICIAL_BUILD)
    return kEnvProduction;
#else
    return kEnvDevelopment;
#endif
}

std::string GetDomain(const std::string& prefix,
                      const std::string& environment) {
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

std::string GetEnvironmentForDomain(const std::string& domain) {
  auto base_domain = GetDomainAndRegistry(
      domain, net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);
  if (base_domain == "brave.com")
    return kEnvProduction;
  if (base_domain == "bravesoftware.com")
    return kEnvStaging;
  if (base_domain == "brave.software")
    return kEnvDevelopment;
  NOTIMPLEMENTED();
  return "";
}

void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterDictionaryPref(prefs::kSkusState);
}

}  // namespace skus
