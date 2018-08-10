/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/alternate_private_search_engine_util.h"

#include "brave/common/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "components/pref_registry/pref_registry_syncable.h"

namespace brave {

bool UseAlternatePrivateSearchEngineEnabled(Profile* profile) {
  return profile->GetOriginalProfile()->GetPrefs()->GetBoolean(
      kUseAlternatePrivateSearchEngine);
}

void ToggleUseAlternatePrivateSearchEngine(Profile* profile) {
  profile->GetOriginalProfile()->GetPrefs()->SetBoolean(
      kUseAlternatePrivateSearchEngine,
      !UseAlternatePrivateSearchEngineEnabled(profile));
}

void RegisterAlternatePrivateSearchEngineProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterBooleanPref(kUseAlternatePrivateSearchEngine, false);
}

}  // namespace brave
