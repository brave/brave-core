/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_PREFS_H_
#define BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_PREFS_H_

class PrefRegistrySimple;
class PrefService;

namespace brave_tabs {

enum TabHoverMode { TOOLTIP = 0, CARD = 1, CARD_WITH_PREVIEW = 2 };

extern const char kTabHoverMode[];

void RegisterBraveProfilePrefs(PrefRegistrySimple* registry);

bool AreTooltipsEnabled(PrefService* prefs);
bool AreCardPreviewsEnabled(PrefService* prefs);
}  // namespace brave_tabs

#endif  // BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_PREFS_H_
