/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PLAYLIST_CORE_COMMON_CONSTANTS_H_
#define BRAVE_COMPONENTS_PLAYLIST_CORE_COMMON_CONSTANTS_H_

namespace playlist {

inline constexpr char kPlaylistExclusionsComponentName[] =
    "Playlist Page Source Exclusions";

inline constexpr char kPlaylistExclusionsComponentId[] =
    "kajcioihdfnnkdjfpciidnmhckaeclmj";

static_assert(sizeof(kPlaylistExclusionsComponentId) == 33u,
              "Component id must be 32 characters plus NUL");

inline constexpr char kPlaylistExclusionsJsonFile[] =
    "playlist_exclusions.json";

}  // namespace playlist

#endif  // BRAVE_COMPONENTS_PLAYLIST_CORE_COMMON_CONSTANTS_H_
