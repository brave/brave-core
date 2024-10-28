/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_NEW_TAB_NEW_TAB_PREFS_H_
#define BRAVE_COMPONENTS_BRAVE_NEW_TAB_NEW_TAB_PREFS_H_

#include <string_view>

class PrefRegistrySimple;
class PrefService;

namespace brave_new_tab::prefs {

// Determines what is displayed when a new tab is opened.
inline constexpr char kNewTabShowsOption[] = "brave.new_tab_page.shows_options";

// Registers Brave new tab profile prefs.
void RegisterProfilePrefs(PrefRegistrySimple* registry);

enum class NewTabShowsOption { kDashboard, kHomepage, kBlankpage };

// Returns a value indicating what is displayed when a new tab is opened.
NewTabShowsOption GetNewTabShowsOption(PrefService* pref_service);

}  // namespace brave_new_tab::prefs

#endif  // BRAVE_COMPONENTS_BRAVE_NEW_TAB_NEW_TAB_PREFS_H_
