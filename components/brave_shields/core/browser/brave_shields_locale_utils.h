// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_BROWSER_BRAVE_SHIELDS_LOCALE_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_BROWSER_BRAVE_SHIELDS_LOCALE_UTILS_H_

#include <string>

class PrefService;

namespace brave_shields {

std::string GetLanguageCodeFromLocale(const std::string& locale);

bool IsAdblockOnlyModeSupportedForLocale(const std::string& locale);

// The following function enables or disables Ad Block Only mode based on the
// locale. If the locale is not supported, it disables Ad Block Only mode and
// sets `brave.shields.adblock_only_mode_was_enabled_for_supported_locale` pref
// value to true.
// If the locale is supported, it enables Ad Block Only mode if it was
// enabled previously for a supported locale.
void ManageAdBlockOnlyModeByLocale(PrefService* local_state,
                                   const std::string& locale);

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_BROWSER_BRAVE_SHIELDS_LOCALE_UTILS_H_
