/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/browser/util.h"

#include "brave/components/playlist/browser/pref_names.h"
#include "components/prefs/pref_service.h"

namespace playlist {

bool IsPlaylistEnabled(const PrefService& prefs) {
  return prefs.GetBoolean(kPlaylistEnabledPref) &&
         !prefs.GetBoolean(kPlaylistDisabledByPolicy);
}

PlaylistEnabledChangeRegistrar::PlaylistEnabledChangeRegistrar() = default;

PlaylistEnabledChangeRegistrar::~PlaylistEnabledChangeRegistrar() = default;

void PlaylistEnabledChangeRegistrar::Init(PrefService* prefs,
                                          base::RepeatingClosure callback) {
  pref_change_registrar_.Init(prefs);
  pref_change_registrar_.Add(kPlaylistEnabledPref, callback);
  pref_change_registrar_.Add(kPlaylistDisabledByPolicy, callback);
}

}  // namespace playlist
