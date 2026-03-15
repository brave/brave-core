/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/skus/browser/skus_utils.h"

#include <utility>

#include "base/check.h"
#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/notimplemented.h"
#include "base/notreached.h"
#include "brave/components/constants/brave_switches.h"
#include "brave/components/skus/browser/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
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

  NOTREACHED() << "Unsupported environment: " << environment;
}

std::string GetEnvironmentForDomain(const std::string& domain) {
  auto base_domain = net::registry_controlled_domains::GetDomainAndRegistry(
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

bool DomainIsForProduct(const std::string& domain, const std::string& product) {
  std::string::size_type index = domain.find(product + ".", 0);
  return index == 0;
}

void RegisterLocalStatePrefs(PrefRegistrySimple* registry) {
  registry->RegisterDictionaryPref(prefs::kSkusState);
  registry->RegisterBooleanPref(prefs::kSkusStateMigratedToLocalState, false);
}

void RegisterProfilePrefsForMigration(PrefRegistrySimple* registry) {
  registry->RegisterDictionaryPref(prefs::kSkusState);
}

void MigrateSkusSettings(PrefService* profile_prefs, PrefService* local_prefs) {
  if (!profile_prefs->HasPrefPath(prefs::kSkusState))
    return;
  base::DictValue obsolete_pref =
      profile_prefs->GetDict(prefs::kSkusState).Clone();
  if (local_prefs->GetBoolean(prefs::kSkusStateMigratedToLocalState)) {
    return;
  }
  local_prefs->Set(prefs::kSkusState, base::Value(std::move(obsolete_pref)));
  local_prefs->SetBoolean(prefs::kSkusStateMigratedToLocalState, true);
  profile_prefs->ClearPref(prefs::kSkusState);
}

bool MaybeImportSkusStateFromCommandLine(PrefService* local_state) {
  const base::CommandLine* command_line =
      base::CommandLine::ForCurrentProcess();

  if (!command_line->HasSwitch(switches::kSkusStateImportPath)) {
    return false;
  }

  base::FilePath import_path =
      command_line->GetSwitchValuePath(switches::kSkusStateImportPath);

  if (import_path.empty()) {
    LOG(ERROR) << "SKUs import: --import-skus-state requires a file path";
    return false;
  }

  if (!base::PathExists(import_path)) {
    LOG(ERROR) << "SKUs import: File not found: " << import_path;
    return false;
  }

  std::string file_contents;
  if (!base::ReadFileToString(import_path, &file_contents)) {
    LOG(ERROR) << "SKUs import: Failed to read file: " << import_path;
    return false;
  }

  auto imported_state =
      base::JSONReader::ReadDict(file_contents, base::JSON_PARSE_RFC);
  if (!imported_state) {
    LOG(ERROR) << "SKUs import: Failed to parse JSON from file: " << import_path;
    return false;
  }

  // The exported state from brave://skus-internals/ has top-level keys like
  // "skus:production", "skus:staging", etc. Each value is a dict containing
  // credentials, orders, etc.
  ScopedDictPrefUpdate state_update(local_state, prefs::kSkusState);
  base::Value::Dict& current_state = state_update.Get();

  int imported_count = 0;
  for (const auto [key, value] : *imported_state) {
    // Only import keys that start with "skus:" to avoid importing
    // unrelated data (like "env" which is also in the export).
    if (!key.starts_with("skus:")) {
      continue;
    }

    // The imported value should be a dict, but we store it as a JSON string
    // in the pref (this matches how the SKU service stores it).
    std::string json_string;
    if (!base::JSONWriter::Write(value, &json_string)) {
      LOG(WARNING) << "SKUs import: Failed to serialize " << key;
      continue;
    }

    current_state.Set(key, json_string);
    imported_count++;
    LOG(INFO) << "SKUs import: Imported " << key;
  }

  if (imported_count > 0) {
    LOG(INFO) << "SKUs import: Successfully imported " << imported_count
              << " environment(s) from " << import_path;
    return true;
  }

  LOG(WARNING) << "SKUs import: No valid SKU state found in file";
  return false;
}

}  // namespace skus
