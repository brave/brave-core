/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/l10n/common/prefs.h"

#include <string_view>

#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace brave_l10n {

namespace {
constexpr std::string_view kObsoleteCountryCode = "brave.country_code";
}  // namespace

void RegisterLocalStatePrefsForMigration(PrefRegistrySimple* registry) {
  // Added 06/2025.
  registry->RegisterStringPref(kObsoleteCountryCode, "");
}

void MigrateObsoleteLocalStatePrefs(PrefService* local_state) {
  // Added 06/2025.
  local_state->ClearPref(kObsoleteCountryCode);
}

}  // namespace brave_l10n
