/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/profiles/profile_manager.h"
#include "components/prefs/pref_service.h"
#include "components/search_engines/search_engines_pref_names.h"

#define PREVENT_OSDD_ADDITION                                                \
  if (!profile->GetPrefs()->GetBoolean(prefs::kAddOpenSearchEngines)) { \
    return;                                                                  \
  }

#include "../../../../../../chrome/browser/ui/search_engines/search_engine_tab_helper.cc"
#undef PREVENT_OSDD_ADDITION
