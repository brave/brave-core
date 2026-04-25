// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/webcompat_reporter/common/pref_names.h"

#include "components/prefs/pref_registry_simple.h"

namespace webcompat_reporter {
namespace prefs {

void RegisterProfilePrefs(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(prefs::kContactInfoSaveFlagPrefs, true);
  registry->RegisterStringPref(prefs::kContactInfoPrefs, "");
}

}  // namespace prefs
}  // namespace webcompat_reporter
