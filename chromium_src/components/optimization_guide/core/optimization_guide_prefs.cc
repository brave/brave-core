/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/optimization_guide/core/model_execution/feature_keys.h"
#include "components/prefs/pref_registry_simple.h"

#define RegisterSettingsEnabledPrefs RegisterSettingsEnabledPrefs_ChromiumImpl

#include <components/optimization_guide/core/optimization_guide_prefs.cc>

#undef RegisterSettingsEnabledPrefs

namespace optimization_guide::prefs {

// Override to default HistorySearch to enabled for Brave
void RegisterSettingsEnabledPrefs(PrefRegistrySimple* registry) {
  for (auto key : kAllUserVisibleFeatureKeys) {
    int default_value =
        (key == UserVisibleFeatureKey::kHistorySearch)
            ? static_cast<int>(FeatureOptInState::kEnabled)
            : static_cast<int>(FeatureOptInState::kNotInitialized);
    registry->RegisterIntegerPref(GetSettingEnabledPrefName(key),
                                  default_value);
  }
}

}  // namespace optimization_guide::prefs
