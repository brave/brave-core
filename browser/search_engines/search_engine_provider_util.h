/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_SEARCH_ENGINES_SEARCH_ENGINE_PROVIDER_UTIL_H_
#define BRAVE_BROWSER_SEARCH_ENGINES_SEARCH_ENGINE_PROVIDER_UTIL_H_

class Profile;
class PrefService;

namespace user_prefs {
class PrefRegistrySyncable;
}  // namespace user_prefs

namespace brave {

bool IsRegionForQwant(Profile* profile);
void SetBraveAsDefaultPrivateSearchProvider(PrefService* prefs);

// For prefs migration.
void RegisterSearchEngineProviderPrefsForMigration(
    user_prefs::PrefRegistrySyncable* registry);
void MigrateSearchEngineProviderPrefs(PrefService* prefs);

// Initialize default provider for private profile.
void PrepareDefaultPrivateSearchProviderDataIfNeeded(Profile* profile);

// Update TemplareURLData with provider guid.
void UpdateDefaultPrivateSearchProviderData(Profile* profile);
void ResetDefaultPrivateSearchProvider(Profile* profile);

}  // namespace brave

#endif  // BRAVE_BROWSER_SEARCH_ENGINES_SEARCH_ENGINE_PROVIDER_UTIL_H_
