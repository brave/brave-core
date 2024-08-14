/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/chrome/browser/ui/search_engines/search_engine_tab_helper.cc"

#include "components/prefs/pref_service.h"
#include "components/search_engines/search_engines_pref_names.h"

// If the current navigation is due to a form submit, a keyword is not
// generated. When the pref is off, always return true so that a keyword is
// never generated.
bool SearchEngineTabHelper::IsFormSubmit(NavigationEntry* entry) {
  Profile* profile =
      Profile::FromBrowserContext(web_contents()->GetBrowserContext());
  if (!profile->GetPrefs()->GetBoolean(prefs::kAddOpenSearchEngines)) {
    return true;
  }
  return ::IsFormSubmit(entry);
}
