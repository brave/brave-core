// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_IOS_CHROME_BROWSER_SHARED_MODEL_PREFS_PREF_NAMES_H_
#define BRAVE_CHROMIUM_SRC_IOS_CHROME_BROWSER_SHARED_MODEL_PREFS_PREF_NAMES_H_

#include "src/ios/chrome/browser/shared/model/prefs/pref_names.h"  // IWYU pragma: export

namespace prefs {
// A boolean specifying whether HTTPS Upgrades are enabled.
inline constexpr char kHttpsUpgradesEnabled[] = "ios.https_upgrades_enabled";
}  // namespace prefs

#endif  // BRAVE_CHROMIUM_SRC_IOS_CHROME_BROWSER_SHARED_MODEL_PREFS_PREF_NAMES_H_
