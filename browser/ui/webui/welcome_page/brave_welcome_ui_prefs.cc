/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/welcome_page/brave_welcome_ui_prefs.h"

#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace brave::welcome_ui::prefs {

namespace {

// Chromium's preference that is being removed (06/2025)
inline constexpr char kHasSeenWelcomePage[] = "browser.has_seen_welcome_page";

}  // namespace

void RegisterProfilePrefs(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(kHasSeenBraveWelcomePage, false);
}

void MigratePrefs(PrefService* prefs) {
  auto* pref = prefs->FindPreference(kHasSeenWelcomePage);
  if (pref && !pref->IsDefaultValue()) {
    prefs->SetBoolean(kHasSeenBraveWelcomePage, pref->GetValue()->GetBool());
  }
}

}  // namespace brave::welcome_ui::prefs
