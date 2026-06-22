/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/prefs.h"

#include "base/check_deref.h"
#include "base/values.h"
#include "brave/components/brave_account/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace brave_account::prefs {

void RegisterPrefs(PrefRegistrySimple* registry) {
  // Registered unconditionally, rather than gated on IsBraveAccountEnabled(),
  // so MigrateObsoleteProfilePrefs() can reach it for every profile. This
  // includes profiles whose state dict was written while the feature was
  // enabled, but is now read while the feature is disabled. Keeping the pref
  // registered lets us heal the dict before the feature is re-enabled. See
  // MigrateObsoleteProfilePrefs().
  CHECK_DEREF(registry).RegisterDictionaryPref(
      kBraveAccountState,
      base::DictValue().Set(keys::kKind, state_kinds::kLoggedOut));
}

void MigrateObsoleteProfilePrefs(PrefService* prefs) {
  // Added 2026-06: kBraveAccountState used to be registered as an empty dict,
  // with an absent `kind` treated as logged-out. It now always carries a
  // `kind`. Stamp it onto pre-existing dicts so the invariant holds for
  // profiles that wrote the pref before the default existed.
  //
  // Runs unconditionally, matching RegisterPrefs(): a dict persisted while the
  // feature was enabled survives on disk after the feature is disabled, and
  // must be healed while disabled so it cannot resurface kind-less on
  // re-enable.
  if (!CHECK_DEREF(prefs).GetDict(kBraveAccountState).FindString(keys::kKind)) {
    ScopedDictPrefUpdate(prefs, kBraveAccountState)
        ->Set(keys::kKind, state_kinds::kLoggedOut);
  }
}

}  // namespace brave_account::prefs
