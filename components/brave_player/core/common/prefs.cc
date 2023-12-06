/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_player/core/common/prefs.h"

#include "components/pref_registry/pref_registry_syncable.h"

namespace brave_player {

void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterDictionaryPref(kBravePlayerAdBlockAdjustmentDisplayedSites);
}

}  // namespace brave_player
