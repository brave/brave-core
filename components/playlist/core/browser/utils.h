/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PLAYLIST_CORE_BROWSER_UTILS_H_
#define BRAVE_COMPONENTS_PLAYLIST_CORE_BROWSER_UTILS_H_

#include "brave/components/playlist/core/common/buildflags/buildflags.h"

static_assert(BUILDFLAG(ENABLE_PLAYLIST));

class PrefService;

namespace playlist {

// Returns true when the Playlist feature flag is enabled and the feature has
// not been disabled by administrator policy (BravePlaylistEnabled managed and
// forced off). A user toggling the pref off via settings does not count as
// "disabled by policy" -- UI remains reachable so the user can flip it back.
bool IsPlaylistAllowed(PrefService* prefs);

}  // namespace playlist

#endif  // BRAVE_COMPONENTS_PLAYLIST_CORE_BROWSER_UTILS_H_
