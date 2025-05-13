// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_PSST_COMMON_PSST_PREFS_H_
#define BRAVE_COMPONENTS_PSST_COMMON_PSST_PREFS_H_

#include "base/component_export.h"
#include "components/prefs/pref_service.h"

class PrefRegistrySimple;

namespace psst {

namespace prefs {
inline constexpr char kPsstSettingsPref[] = "brave.psst.settings";
inline constexpr char kPsstEnabled[] = "brave.psst.settings.enable_psst";
}  // namespace prefs

COMPONENT_EXPORT(PSST_COMMON)
void RegisterProfilePrefs(PrefRegistrySimple* registry);

COMPONENT_EXPORT(PSST_COMMON)
bool GetEnablePsstFlag(PrefService* prefs);

COMPONENT_EXPORT(PSST_COMMON)
void SetEnablePsstFlag(PrefService* prefs, const bool val);

}  // namespace psst

#endif  // BRAVE_COMPONENTS_PSST_COMMON_PSST_PREFS_H_
