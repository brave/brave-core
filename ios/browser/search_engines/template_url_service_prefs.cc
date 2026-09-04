// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/search_engines/template_url_service_prefs.h"

#include "components/pref_registry/pref_registry_syncable.h"
#include "components/search_engines/search_engines_pref_names.h"

namespace search_engines {

void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterStringPref(prefs::kSyncedDefaultPrivateSearchProviderGUID,
                               std::string(),
                               user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
  registry->RegisterDictionaryPref(
      prefs::kSyncedDefaultPrivateSearchProviderData,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
}

}  // namespace search_engines
