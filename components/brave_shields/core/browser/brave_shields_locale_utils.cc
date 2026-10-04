// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/core/browser/brave_shields_locale_utils.h"

#include "base/feature_list.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_shields/core/common/brave_shield_constants.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/components/brave_shields/core/common/pref_names.h"
#include "components/prefs/pref_service.h"

namespace brave_shields {

std::string GetLanguageCodeFromLocale(const std::string& locale) {
  const std::string::size_type loc = locale.find("-");
  if (loc == std::string::npos) {
    return base::ToLowerASCII(locale);
  }
  return base::ToLowerASCII(locale.substr(0, loc));
}

bool IsAdblockOnlyModeSupportedForLocale(const std::string& locale) {
  return kAdblockOnlyModeSupportedLanguageCodes.contains(
      GetLanguageCodeFromLocale(locale));
}

void ManageAdBlockOnlyModeByLocale(PrefService* local_state,
                                   const std::string& locale) {
  if (!base::FeatureList::IsEnabled(
          brave_shields::features::kAdblockOnlyMode)) {
    return;
  }

  if (!IsAdblockOnlyModeSupportedForLocale(locale)) {
    // If the current locale is not supported and Ad Block Only mode is enabled,
    // then disable Ad Block Only mode and set
    // `brave.shields.adblock_only_mode_was_enabled_for_supported_locale` pref
    // value to true.
    if (local_state->GetBoolean(prefs::kAdBlockOnlyModeEnabled)) {
      local_state->SetBoolean(prefs::kAdBlockOnlyModeEnabled, false);
      local_state->SetBoolean(
          prefs::kAdBlockOnlyModeWasEnabledForSupportedLocale, true);
    }
  } else {
    // If the current locale is supported, Ad Block Only mode is not enabled and
    // was enabled previously for a supported locale, enable it.
    if (!local_state->GetBoolean(prefs::kAdBlockOnlyModeEnabled) &&
        local_state->GetBoolean(
            prefs::kAdBlockOnlyModeWasEnabledForSupportedLocale)) {
      local_state->SetBoolean(prefs::kAdBlockOnlyModeEnabled, true);
      local_state->SetBoolean(
          prefs::kAdBlockOnlyModeWasEnabledForSupportedLocale, false);
    }
  }
}

}  // namespace brave_shields
