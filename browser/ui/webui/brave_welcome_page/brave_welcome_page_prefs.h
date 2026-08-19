/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_WELCOME_PAGE_BRAVE_WELCOME_PAGE_PREFS_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_WELCOME_PAGE_BRAVE_WELCOME_PAGE_PREFS_H_

class PrefRegistrySimple;
class PrefService;

namespace brave_welcome_page::prefs {

inline constexpr char kHasSeenBraveWelcomePage[] =
    "brave.has_seen_brave_welcome_page";

void RegisterProfilePrefs(PrefRegistrySimple* registry);
void MigratePrefs(PrefService* prefs);

}  // namespace brave_welcome_page::prefs

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_WELCOME_PAGE_BRAVE_WELCOME_PAGE_PREFS_H_
