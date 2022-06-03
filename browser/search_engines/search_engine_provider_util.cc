/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/search_engines/search_engine_provider_util.h"

#include <memory>
#include <vector>

#include "brave/components/constants/pref_names.h"
#include "brave/components/search_engines/brave_prepopulated_engines.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "components/search_engines/search_engines_pref_names.h"
#include "components/search_engines/template_url_data.h"
#include "components/search_engines/template_url_prepopulate_data.h"
#include "components/search_engines/template_url_service.h"

namespace brave {

bool IsRegionForQwant(Profile* profile) {
  return TemplateURLPrepopulateData::GetPrepopulatedDefaultSearch(
             profile->GetPrefs())
             ->prepopulate_id ==
         TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_QWANT;
}

void RegisterSearchEngineProviderPrefsForMigration(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterBooleanPref(kUseAlternativeSearchEngineProvider, false);
  registry->RegisterBooleanPref(kShowAlternativeSearchEngineProviderToggle,
                                false);
}

void MigrateSearchEngineProviderPrefs(Profile* profile) {
  auto* preference = profile->GetPrefs()->FindPreference(
      prefs::kSyncedDefaultPrivateSearchProviderGUID);
  if (!preference->IsDefaultValue())
    return;

  const bool need_migrate =
      profile->GetPrefs()->GetBoolean(
          kShowAlternativeSearchEngineProviderToggle) &&
      profile->GetPrefs()->GetBoolean(kUseAlternativeSearchEngineProvider);

  if (!need_migrate)
    return;

  // If the user has been using DDG in private profile, set DDG as a initial
  // provider.
  std::vector<TemplateURLPrepopulateData::BravePrepopulatedEngineID>
      alt_search_providers = {
          TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_DUCKDUCKGO,
          TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_DUCKDUCKGO_DE,
          TemplateURLPrepopulateData::
              PREPOPULATED_ENGINE_ID_DUCKDUCKGO_AU_NZ_IE};

  std::unique_ptr<TemplateURLData> data;
  for (const auto& id : alt_search_providers) {
    data = TemplateURLPrepopulateData::GetPrepopulatedEngine(
        profile->GetPrefs(), id);
    if (data)
      break;
  }

  // There should ALWAYS be one entry
  DCHECK(data);
  profile->GetPrefs()->SetString(prefs::kSyncedDefaultPrivateSearchProviderGUID,
                                 data->sync_guid);

  // From now on, user will not see DDG toggle button and can control search
  // provider for private window via settings.
  profile->GetPrefs()->ClearPref(kShowAlternativeSearchEngineProviderToggle);
  profile->GetPrefs()->ClearPref(kUseAlternativeSearchEngineProvider);
}

void SetDefaultPrivateSearchProvider(Profile* profile) {
  // Set normal profile's default provider to a private profile's
  // default provider by default.
  auto* preference = profile->GetPrefs()->FindPreference(
      prefs::kSyncedDefaultPrivateSearchProviderGUID);

  if (!preference || !preference->IsDefaultValue())
    return;

  // Set default provider.
  auto* service = TemplateURLServiceFactory::GetForProfile(profile);
  profile->GetPrefs()->SetString(
      prefs::kSyncedDefaultPrivateSearchProviderGUID,
      service->GetDefaultSearchProvider()->sync_guid());
}

void ClearDefaultPrivateSearchProvider(Profile* profile) {
  PrefService* prefs = profile->GetPrefs();
  prefs->ClearPref(prefs::kSyncedDefaultPrivateSearchProviderGUID);
}

}  // namespace brave
