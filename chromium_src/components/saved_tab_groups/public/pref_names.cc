/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/saved_tab_groups/public/pref_names.h"

#include "components/pref_registry/pref_registry_syncable.h"

#define RegisterProfilePrefs RegisterProfilePrefs_ChromiumImpl
#include <components/saved_tab_groups/public/pref_names.cc>
#undef RegisterProfilePrefs

namespace tab_groups::prefs {

void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry) {
  RegisterProfilePrefs_ChromiumImpl(registry);

#if BUILDFLAG(IS_ANDROID)
  registry->SetDefaultPrefValue(prefs::kAutoOpenSyncedTabGroups,
                                base::Value(false));
#endif  // BUILDFLAG(IS_ANDROID)
}

}  // namespace tab_groups::prefs
