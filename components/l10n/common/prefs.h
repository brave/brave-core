/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_L10N_COMMON_PREFS_H_
#define BRAVE_COMPONENTS_L10N_COMMON_PREFS_H_

class PrefRegistrySimple;

namespace brave_l10n {

namespace prefs {

// The country code used to register component resources.
inline constexpr char kCountryCode[] = "brave.country_code";

}  // namespace prefs

void RegisterL10nLocalStatePrefs(PrefRegistrySimple* registry);

}  // namespace brave_l10n

#endif  // BRAVE_COMPONENTS_L10N_COMMON_PREFS_H_
