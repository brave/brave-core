// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/core/common/pref_names.h"

#include "components/pref_registry/pref_registry_syncable.h"

namespace brave_shields {

void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterBooleanPref(prefs::kFBEmbedControlType, true);
  registry->RegisterBooleanPref(prefs::kTwitterEmbedControlType, true);
  registry->RegisterBooleanPref(prefs::kLinkedInEmbedControlType, false);
  registry->RegisterBooleanPref(prefs::kAdBlockDeveloperMode, false);

  registry->RegisterIntegerPref(prefs::kAdblockShieldsDisabledCount, 0);
  registry->RegisterBooleanPref(prefs::kAdblockAdBlockOnlyModeEnabled, false);
  registry->RegisterBooleanPref(prefs::kAdblockAdBlockOnlyModePromptDismissed,
                                false);
}

}  // namespace brave_shields
