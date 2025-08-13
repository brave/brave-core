/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ORIGIN_PREF_NAMES_H_
#define BRAVE_COMPONENTS_BRAVE_ORIGIN_PREF_NAMES_H_

class PrefRegistrySimple;

namespace brave_origin::prefs {

// Dictionary pref containing policy settings for Brave Origin users.
inline constexpr char kBraveOriginPolicySettings[] =
    "brave_origin.policy_settings";

// Registers preferences for the Brave Origin component.
void RegisterLocalStatePrefs(PrefRegistrySimple* registry);

}  // namespace brave_origin::prefs

#endif  // BRAVE_COMPONENTS_BRAVE_ORIGIN_PREF_NAMES_H_
