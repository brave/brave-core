// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/containers/core/browser/prefs_registration.h"

#include "brave/components/containers/core/browser/pref_names.h"
#include "components/pref_registry/pref_registry_syncable.h"

namespace containers {

void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterListPref(prefs::kContainersList,
                             user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
}

}  // namespace containers
