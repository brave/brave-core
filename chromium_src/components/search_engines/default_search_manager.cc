/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/search_engines/search_engines_pref_names.h"

// When extension is loaded and it provides search provider as a default
// provider, that search engine data is stored in
// prefs(kDefaultSearchProviderDataPrefName) as an extension controlled
// pref(See SettingsOverridesAPI::SetPref()).
// And this provider is added to TemplateURLService.
// The problem is only normal profile's TemplateURLService can know this because
// same SettingsOverridesAPI instance is use by normal and incognito. It only
// asks to add extension's provider to normal profile's TemplateURLService.
// Because of this, private(also tor) window doesn't know it. To resolve this,
// SearchEngineProviderService::UseExtensionSearchProvider() explicitely adds
// that provider when normal window uses search provider from extension and set
// provider data to kDefaultSearchProviderDataPrefName.

// Why we need this patch? DefaultSearchManager determines whether current
// default provider comes from extension or user by checking this pref is
// extension controlled or not(See |if (pref->IsExtensionControlled())| in
// DefaultSearchManager::LoadDefaultSearchEngineFromPrefs()).
// This prefs set by SearchEngineProviderService is not extension controlled.
// To make it extension-controlled, we should set it via PreferenceAPI.
// However, it also uses same instance for normal/private profile. To resolve
// this another |kDefaultSearchProviderByExtension| pref is used.
// This is needed only when extension's `Allow in private` option is off.
// When this option is on, |kDefaultSearchProviderDataPrefName| is synced with
// normal profile. So, |kDefaultSearchProviderDataPrefName| can act as an
// extension controlled prefs.

// clang-format off
#define LOADDEFAULTSEARCHENGINEFROMPREFS_BRAVE                 \
  } else if (pref_service_->GetBoolean(                        \
                 prefs::kDefaultSearchProviderByExtension)) {  \
    extension_default_search_ = std::move(turl_data);
// clang-format on

#include "../../../../components/search_engines/default_search_manager.cc"

#undef LOADDEFAULTSEARCHENGINEFROMPREFS_BRAVE
