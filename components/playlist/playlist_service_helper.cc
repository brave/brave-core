/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/playlist_service_helper.h"

#include <vector>

#include "brave/components/playlist/playlist_constants.h"
#include "brave/components/playlist/playlist_types.h"

namespace playlist {

base::Value::Dict GetValueFromPlaylistItemInfo(const PlaylistItemInfo& info) {
  base::Value::Dict playlist_value;
  playlist_value.Set(kPlaylistItemIDKey, info.id);
  playlist_value.Set(kPlaylistItemTitleKey, info.title);
  playlist_value.Set(kPlaylistItemThumbnailPathKey, info.thumbnail_path);
  playlist_value.Set(kPlaylistItemMediaFilePathKey, info.media_file_path);
  playlist_value.Set(kPlaylistItemReadyKey, info.ready);
  return playlist_value;
}

}  // namespace playlist
