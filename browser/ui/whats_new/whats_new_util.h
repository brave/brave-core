/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WHATS_NEW_WHATS_NEW_UTIL_H_
#define BRAVE_BROWSER_UI_WHATS_NEW_WHATS_NEW_UTIL_H_

class PrefRegistrySimple;
class PrefService;
class Browser;

namespace whats_new {

// Returns true when we want to show whats-new page in foreground tab.
bool ShouldShowBraveWhatsNewForState(PrefService* local_state);
void RegisterLocalStatePrefs(PrefRegistrySimple* registry);
void StartBraveWhatsNew(Browser* browser);
void SetCurrentVersionForTesting(double major_version);

}  // namespace whats_new

#endif  // BRAVE_BROWSER_UI_WHATS_NEW_WHATS_NEW_UTIL_H_
