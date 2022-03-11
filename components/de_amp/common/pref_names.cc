/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/de_amp/common/pref_names.h"

#include "components/prefs/pref_registry_simple.h"

namespace de_amp {

const char kDeAmpPrefEnabled[] = "brave.de_amp.enabled";

void RegisterProfilePrefs(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(kDeAmpPrefEnabled, true);  // default on
}

}  // namespace de_amp
