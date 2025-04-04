/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/youtube_script_injector/common/pref_names.h"

#include "components/prefs/pref_registry_simple.h"

namespace youtube_script_injector::prefs {

void RegisterProfilePrefs(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(kYouTubeBackgroundVideoPlaybackEnabled, true);
  registry->RegisterBooleanPref(kYouTubeExtraControlsEnabled, true);
}

}  // youtube_script_injector::prefs
