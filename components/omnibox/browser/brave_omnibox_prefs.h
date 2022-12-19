/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_OMNIBOX_BROWSER_BRAVE_OMNIBOX_PREFS_H_
#define BRAVE_COMPONENTS_OMNIBOX_BROWSER_BRAVE_OMNIBOX_PREFS_H_

class PrefRegistrySimple;

namespace omnibox {

extern const char kAutocompleteEnabled[];
extern const char kTopSiteSuggestionsEnabled[];
extern const char kHistorySuggestionsEnabled[];
extern const char kBookmarkSuggestionsEnabled[];

void RegisterBraveProfilePrefs(PrefRegistrySimple* registry);

}  // namespace omnibox

#endif  // BRAVE_COMPONENTS_OMNIBOX_BROWSER_BRAVE_OMNIBOX_PREFS_H_
