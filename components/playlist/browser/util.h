/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PLAYLIST_BROWSER_UTIL_H_
#define BRAVE_COMPONENTS_PLAYLIST_BROWSER_UTIL_H_

#include "base/functional/callback.h"
#include "components/prefs/pref_change_registrar.h"

class PrefService;

namespace playlist {

// Returns true if playlist is enabled by user and not disabled by policy.
bool IsPlaylistEnabled(const PrefService& prefs);

// A helper class that monitors both playlist enabled preferences
// and calls a callback when either changes.
class PlaylistEnabledChangeRegistrar {
 public:
  PlaylistEnabledChangeRegistrar();
  ~PlaylistEnabledChangeRegistrar();

  void Init(PrefService* prefs, base::RepeatingClosure callback);

 private:
  PrefChangeRegistrar pref_change_registrar_;
};

}  // namespace playlist

#endif  // BRAVE_COMPONENTS_PLAYLIST_BROWSER_UTIL_H_
