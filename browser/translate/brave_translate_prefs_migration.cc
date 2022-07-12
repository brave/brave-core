/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/translate/brave_translate_prefs_migration.h"

#include "brave/components/translate/core/common/brave_translate_features.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/translate/core/browser/translate_pref_names.h"

namespace translate {

namespace prefs {
const char kMigratedToInternalTranslation[] =
    "brave.translate_migrated_from_extension";
}  // namespace prefs

void RegisterBraveProfilePrefsForMigration(PrefRegistrySimple* registry) {
  // No sync for migration prefs
  registry->RegisterBooleanPref(prefs::kMigratedToInternalTranslation, false);
}

void MigrateBraveProfilePrefs(PrefService* prefs) {
  if (!IsBraveTranslateGoAvailable())
    return;
  if (prefs->GetBoolean(prefs::kMigratedToInternalTranslation))
    return;  // Already migrated

  prefs->SetBoolean(prefs::kMigratedToInternalTranslation, true);

  // TODO(matuchin): make kOfferTranslateEnabled syncable again when the
  // migration is finished.
  prefs->ClearPref(prefs::kOfferTranslateEnabled);
}

}  // namespace translate
