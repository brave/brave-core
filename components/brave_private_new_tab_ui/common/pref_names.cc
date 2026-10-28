/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_private_new_tab_ui/common/pref_names.h"

#include "components/prefs/pref_registry_simple.h"

namespace brave_private_new_tab::prefs {

void RegisterProfilePrefs(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(kBravePrivateWindowDisclaimerDismissed, false);
  registry->RegisterBooleanPref(kBraveTorWindowDisclaimerDismissed, false);
}

}  // namespace brave_private_new_tab::prefs
