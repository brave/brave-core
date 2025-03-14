// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_PSST_COMMON_PSST_PREFS_H_
#define BRAVE_COMPONENTS_PSST_COMMON_PSST_PREFS_H_

#include <string>

#include "base/component_export.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/user_prefs/user_prefs.h"

namespace psst {

namespace prefs {
constexpr char kPsstSettingsPref[] = "brave.psst.settings";
}  // namespace prefs

enum PsstConsentStatus {
  kAsk,     // show the popup dialog to ask user to apply privacy
  kAllow,   // continue to apply privacy with no prompts
  kBlock    // do not ask user any more
};

struct PsstSettings {
  PsstConsentStatus consent_status;
  int script_version;
};

// This is a dictionary of PSST settings.
// The key is a string of the form "<rule_name>:<username>".
// {
//   "twitter.<username>" : {
//     "consent_status": 1, // integer, corresponds to PsstConsentStatus enum.
//     "script_version": 1, // integer, last inserted script version.
//   }
// }

COMPONENT_EXPORT(PSST_COMMON)
void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

COMPONENT_EXPORT(PSST_COMMON)
std::optional<PsstSettings> GetPsstSettings(const std::string& user_id,
                                            const std::string& name,
                                            PrefService* prefs);
COMPONENT_EXPORT(PSST_COMMON)
base::Value* SetPsstSettings(const std::string& user_id,
                             const std::string& name,
                             const PsstSettings settings,
                             PrefService* prefs);

}  // namespace psst

#endif  // BRAVE_COMPONENTS_PSST_COMMON_PSST_PREFS_H_
