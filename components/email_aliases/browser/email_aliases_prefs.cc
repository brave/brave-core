/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/email_aliases/browser/email_aliases_prefs.h"

#include <string>

#include "brave/components/email_aliases/browser/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/sync_preferences/pref_service_syncable.h"

namespace email_aliases {

using user_prefs::PrefRegistrySyncable;

void RegisterProfilePrefs(PrefRegistrySyncable* registry) {
  registry->RegisterStringPref(kEmailAliasesVerificationToken, std::string());
  registry->RegisterStringPref(kEmailAliasesAuthToken, std::string());
  registry->RegisterStringPref(kEmailAliasesAccountEmail, std::string());
  registry->RegisterDictionaryPref(kEmailAliasesNotes, base::Value::Dict(),
                                   PrefRegistrySyncable::SYNCABLE_PREF);
}

}  // namespace email_aliases
