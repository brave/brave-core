/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/gcm_driver/brave_gcm_utils.h"

#include "base/values.h"
#include "brave/common/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "components/pref_registry/pref_registry_syncable.h"

namespace gcm {

// Chromium pref deprecated as of 01/2020.
const char kGCMChannelStatus[] = "gcm.channel_status";

void RegisterGCMProfilePrefs(user_prefs::PrefRegistrySyncable* registry) {
  // Deprecated Chromium pref.
  registry->RegisterBooleanPref(kGCMChannelStatus, false);
  // Current Brave equivalent of the deprecated pref.
  registry->RegisterBooleanPref(kBraveGCMChannelStatus, false);
}

void MigrateGCMPrefs(Profile* profile) {
  PrefService* prefs = profile->GetPrefs();
  // The default was false (see above).
  auto* pref = prefs->FindPreference(kGCMChannelStatus);
  if (pref && !pref->IsDefaultValue())
    prefs->SetBoolean(kBraveGCMChannelStatus, pref->GetValue()->GetBool());
  prefs->ClearPref(kGCMChannelStatus);
}

}  // namespace gcm
