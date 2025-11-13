/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/themes/brave_dark_mode_utils.h"

#include "brave/components/constants/pref_names.h"
#include "components/prefs/pref_registry_simple.h"

namespace dark_mode {

void RegisterBraveDarkModeLocalStatePrefs(PrefRegistrySimple* registry) {
  registry->RegisterIntegerPref(
      kBraveDarkMode,
      static_cast<int>(BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DEFAULT));
}

}  // namespace dark_mode
