/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PLAYLIST_BROWSER_TYPE_CONVERTER_H_
#define BRAVE_COMPONENTS_PLAYLIST_BROWSER_TYPE_CONVERTER_H_

#include "base/values.h"
#include "brave/components/playlist/browser/playlist_types.h"
#include "brave/components/playlist/common/mojom/playlist.mojom.h"

namespace playlist {

bool IsItemValueMalformed(const base::Value::Dict& dict);

// Converters between mojom::PlaylistItem and base::Value --------------------
mojom::PlaylistItemPtr ConvertValueToPlaylistItem(
    const base::Value::Dict& dict);
base::Value::Dict ConvertPlaylistItemToValue(
    const mojom::PlaylistItemPtr& item);

// Converters between mojom::PlaylistItemList and base::Value ----------------
// Note that Playlist value only contains the ids of its
// children. The actual value of the children is stored in a separate value.
// This is to make playlist items can be shared by multiple playlists. For
// more details, please see a comment in playlist/pref_names.h
mojom::PlaylistPtr ConvertValueToPlaylist(
    const base::Value::Dict& playlist_dict,
    const base::Value::Dict& items_dict);
base::Value::Dict ConvertPlaylistToValue(const mojom::PlaylistPtr& playlist);

}  // namespace playlist

#endif  // BRAVE_COMPONENTS_PLAYLIST_BROWSER_TYPE_CONVERTER_H_
