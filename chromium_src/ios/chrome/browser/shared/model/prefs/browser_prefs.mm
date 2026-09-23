/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"

namespace brave {
void RegisterBrowserStatePrefs(user_prefs::PrefRegistrySyncable* registry);
void RegisterLocalStatePrefs(PrefRegistrySimple* registry);
void MigrateObsoleteProfilePrefs(PrefService* prefs);
void MigrateObsoleteLocalStatePrefs(PrefService* prefs);
}  // namespace brave

#include <ios/chrome/browser/shared/model/prefs/browser_prefs.mm>
