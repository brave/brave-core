/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SEARCH_COMMON_PREF_NAMES_H_
#define BRAVE_COMPONENTS_BRAVE_SEARCH_COMMON_PREF_NAMES_H_

namespace brave_search::prefs {

// Set when the `newtab_v1` NTP searchbox source was enabled via the day zero
// experiment at install time, so that it remains enabled afterwards.
inline constexpr char kNewTabV1SourceEnabledAtFirstRun[] =
    "brave.brave_search.new_tab_v1_first_run";

}  // namespace brave_search::prefs

#endif  // BRAVE_COMPONENTS_BRAVE_SEARCH_COMMON_PREF_NAMES_H_
