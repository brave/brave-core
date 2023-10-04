/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/search_engines/search_engine_provider_util.h"

#include <memory>
#include <string>
#include <vector>

#include "brave/browser/search_engines/pref_names.h"
#include "brave/components/search_engines/brave_prepopulated_engines.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "components/search_engines/search_engines_pref_names.h"
#include "components/search_engines/template_url_data.h"
#include "components/search_engines/template_url_data_util.h"
#include "components/search_engines/template_url_prepopulate_data.h"
#include "components/search_engines/template_url_service.h"

namespace brave {

void SetBraveAsDefaultPrivateSearchProvider(PrefService* prefs) {
  auto data = TemplateURLPrepopulateData::GetPrepopulatedEngine(
      prefs, TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE);
  DCHECK(data);
  prefs->SetString(prefs::kSyncedDefaultPrivateSearchProviderGUID,
                   data->sync_guid);
  prefs->SetDict(prefs::kSyncedDefaultPrivateSearchProviderData,
                 TemplateURLDataToDictionary(*data));
}

bool IsRegionForQwant(Profile* profile) {
  return TemplateURLPrepopulateData::GetPrepopulatedDefaultSearch(
             profile->GetPrefs())
             ->prepopulate_id ==
         TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_QWANT;
}

void RegisterSearchEngineProviderPrefsForMigration(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterBooleanPref(kUseAlternativePrivateSearchEngineProvider,
                                false);
  registry->RegisterBooleanPref(
      kShowAlternativePrivateSearchEngineProviderToggle, false);
}

void MigrateSearchEngineProviderPrefs(PrefService* prefs) {
  const bool need_migrate =
      prefs->GetBoolean(kShowAlternativePrivateSearchEngineProviderToggle) &&
      prefs->GetBoolean(kUseAlternativePrivateSearchEngineProvider);

  if (!need_migrate) {
    // Clear and early return when doesn't need migration.
    prefs->ClearPref(kShowAlternativePrivateSearchEngineProviderToggle);
    prefs->ClearPref(kUseAlternativePrivateSearchEngineProvider);
    return;
  }

  // If the user has been using DDG in private profile, set DDG as a initial
  // provider.
  static constexpr TemplateURLPrepopulateData::BravePrepopulatedEngineID
      alt_search_providers[] = {
          TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_DUCKDUCKGO,
          TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_DUCKDUCKGO_DE,
          TemplateURLPrepopulateData::
              PREPOPULATED_ENGINE_ID_DUCKDUCKGO_AU_NZ_IE};

  std::unique_ptr<TemplateURLData> data;
  for (const auto& id : alt_search_providers) {
    data = TemplateURLPrepopulateData::GetPrepopulatedEngine(prefs, id);
    if (data)
      break;
  }

  // There should ALWAYS be one entry
  DCHECK(data);
  prefs->SetString(prefs::kSyncedDefaultPrivateSearchProviderGUID,
                   data->sync_guid);
  prefs->SetDict(prefs::kSyncedDefaultPrivateSearchProviderData,
                 TemplateURLDataToDictionary(*data.get()));

  // From now on, user will not see DDG toggle button and can control search
  // provider for private window via settings.
  prefs->ClearPref(kShowAlternativePrivateSearchEngineProviderToggle);
  prefs->ClearPref(kUseAlternativePrivateSearchEngineProvider);
}

void UpdateDefaultPrivateSearchProviderData(Profile* profile) {
  auto* service = TemplateURLServiceFactory::GetForProfile(profile);
  DCHECK(service->loaded());

  auto* prefs = profile->GetPrefs();
  const std::string private_provider_guid =
      prefs->GetString(prefs::kSyncedDefaultPrivateSearchProviderGUID);

  if (private_provider_guid.empty()) {
    // This can happen while resetting whole settings.
    // In this case, set brave as a default search provider.
    SetBraveAsDefaultPrivateSearchProvider(prefs);
    return;
  }

  // Sync kSyncedDefaultPrivateSearchProviderData with newly updated provider's
  // one.
  if (auto* url = service->GetTemplateURLForGUID(private_provider_guid)) {
    prefs->SetDict(prefs::kSyncedDefaultPrivateSearchProviderData,
                   TemplateURLDataToDictionary(url->data()));
    return;
  }

  // When user delete current private search provder from provider list in
  // settings page, |private_provider_guid| will not be existed in the list. Use
  // Brave.
  SetBraveAsDefaultPrivateSearchProvider(prefs);
}

void PrepareDefaultPrivateSearchProviderDataIfNeeded(Profile* profile) {
  auto* prefs = profile->GetPrefs();
  auto* preference =
      prefs->FindPreference(prefs::kSyncedDefaultPrivateSearchProviderGUID);

  if (!preference)
    return;

  auto* service = TemplateURLServiceFactory::GetForProfile(profile);
  DCHECK(service->loaded());

  const std::string private_provider_guid =
      prefs->GetString(prefs::kSyncedDefaultPrivateSearchProviderGUID);

  // Set Brave as a private window's initial search provider.
  if (private_provider_guid.empty()) {
    SetBraveAsDefaultPrivateSearchProvider(prefs);
    return;
  }

  preference =
      prefs->FindPreference(prefs::kSyncedDefaultPrivateSearchProviderData);
  // Cache if url data is not yet existed.
  if (preference->IsDefaultValue()) {
    if (auto* url = service->GetTemplateURLForGUID(private_provider_guid)) {
      prefs->SetDict(prefs::kSyncedDefaultPrivateSearchProviderData,
                     TemplateURLDataToDictionary(url->data()));
    } else {
      // This could happen with update default provider list when brave is not
      // updated for longtime. So it doesn't have any chance to cache url data.
      // Set Brave as default private search provider.
      SetBraveAsDefaultPrivateSearchProvider(prefs);
    }
    return;
  }

  // In this case previous default private search provider doesn't exist in
  // current default provider list. This could happen when default provider list
  // is updated. Add previous provider to service.
  if (auto* url = service->GetTemplateURLForGUID(private_provider_guid); !url) {
    auto private_url_data =
        TemplateURLDataFromDictionary(preference->GetValue()->GetDict());
    private_url_data->id = kInvalidTemplateURLID;
    DCHECK_EQ(private_provider_guid, private_url_data->sync_guid);
    service->Add(std::make_unique<TemplateURL>(*private_url_data));
  }
}

void ResetDefaultPrivateSearchProvider(Profile* profile) {
  PrefService* prefs = profile->GetPrefs();
  prefs->ClearPref(prefs::kSyncedDefaultPrivateSearchProviderGUID);
  prefs->ClearPref(prefs::kSyncedDefaultPrivateSearchProviderData);

  PrepareDefaultPrivateSearchProviderDataIfNeeded(profile);
}

}  // namespace brave
