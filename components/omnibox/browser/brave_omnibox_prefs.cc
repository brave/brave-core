/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/omnibox/browser/brave_omnibox_prefs.h"

#include "components/prefs/pref_registry_simple.h"

namespace omnibox {

void RegisterBraveProfilePrefs(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(kAutocompleteEnabled, true);
  registry->RegisterBooleanPref(kTopSiteSuggestionsEnabled, true);
  registry->RegisterBooleanPref(kHistorySuggestionsEnabled, true);
  registry->RegisterBooleanPref(kBookmarkSuggestionsEnabled, true);
  registry->RegisterBooleanPref(kCommanderSuggestionsEnabled, true);
}

}  // namespace omnibox
