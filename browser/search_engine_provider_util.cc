/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/search_engine_provider_util.h"

#include "brave/browser/guest_window_search_engine_provider_controller.h"
#include "brave/browser/private_window_search_engine_provider_controller.h"
#include "brave/browser/tor_window_search_engine_provider_controller.h"
#include "brave/common/pref_names.h"
#include "brave/components/search_engines/brave_prepopulated_engines.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "components/pref_registry/pref_registry_syncable.h"

namespace brave {

bool UseAlternativeSearchEngineProviderEnabled(Profile* profile) {
  return profile->GetOriginalProfile()->GetPrefs()->GetBoolean(
      kUseAlternativeSearchEngineProvider);
}

void ToggleUseAlternativeSearchEngineProvider(Profile* profile) {
  profile->GetOriginalProfile()->GetPrefs()->SetBoolean(
      kUseAlternativeSearchEngineProvider,
      !UseAlternativeSearchEngineProviderEnabled(profile));
}

void RegisterAlternativeSearchEngineProviderProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterBooleanPref(kUseAlternativeSearchEngineProvider, false);
  registry->RegisterIntegerPref(
      kAlternativeSearchEngineProviderInTor,
      TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_DUCKDUCKGO);
}

void InitializeSearchEngineProviderIfNeeded(Profile* profile) {
  // These search engine provider will be destroyed when observing template url
  // service is terminated.
  // TODO(simonhong): Refactor these controller with KeyedService.
  if (profile->GetProfileType() == Profile::INCOGNITO_PROFILE) {
    new PrivateWindowSearchEngineProviderController(profile);
    return;
  }

  if (profile->IsTorProfile() &&
      profile->GetProfileType() == Profile::GUEST_PROFILE) {
    new TorWindowSearchEngineProviderController(profile);
    return;
  }

  if (profile->GetProfileType() == Profile::GUEST_PROFILE) {
    new GuestWindowSearchEngineProviderController(profile);
    return;
  }
}

}  // namespace brave
