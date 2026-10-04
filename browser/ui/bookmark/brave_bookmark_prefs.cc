/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/bookmark/brave_bookmark_prefs.h"

#include "components/prefs/pref_registry_simple.h"

namespace brave::bookmarks::prefs {

void RegisterProfilePrefs(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(kShowAllBookmarksButton, true);
}

}  // namespace brave::bookmarks::prefs
