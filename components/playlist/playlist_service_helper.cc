/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/playlist_service_helper.h"

#include <vector>

#include "base/json/values_util.h"
#include "brave/components/playlist/playlist_constants.h"
#include "brave/components/playlist/playlist_types.h"

namespace playlist {

base::Value::Dict GetValueFromPlaylistItemInfo(const PlaylistItemInfo& info) {
  base::Value::Dict playlist_value;
  playlist_value.Set(kPlaylistItemIDKey, info.id);
  playlist_value.Set(kPlaylistItemTitleKey, info.title);
  playlist_value.Set(kPlaylistItemPageSrcKey, info.page_src);
  playlist_value.Set(kPlaylistItemMediaSrcKey, info.media_src);
  playlist_value.Set(kPlaylistItemThumbnailSrcKey, info.thumbnail_src);
  playlist_value.Set(kPlaylistItemThumbnailPathKey, info.thumbnail_path);
  playlist_value.Set(kPlaylistItemMediaFilePathKey, info.media_file_path);
  playlist_value.Set(kPlaylistItemMediaFileCachedKey, info.media_file_cached);
  playlist_value.Set(kPlaylistItemAuthorKey, info.author);
  playlist_value.Set(kPlaylistItemDurationKey,
                     base::TimeDeltaToValue(info.duration));
  return playlist_value;
}

}  // namespace playlist
