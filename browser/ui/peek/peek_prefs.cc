/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/peek/peek_prefs.h"

#include "components/prefs/pref_registry_simple.h"

namespace peek::prefs {

void RegisterProfilePrefs(PrefRegistrySimple* registry) {
  registry->RegisterDoublePref(kCardWidthRatio, 0);
  registry->RegisterDoublePref(kCardHeightRatio, 0);
}

}  // namespace peek::prefs
