/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/playlist_service_helper.h"

#include <vector>

#include "base/values.h"
#include "brave/components/playlist/playlist_constants.h"
#include "brave/components/playlist/playlist_types.h"

namespace playlist {

base::Value GetValueFromPlaylistItemInfo(const PlaylistItemInfo& info) {
  base::Value playlist_value(base::Value::Type::DICTIONARY);
  playlist_value.SetStringKey(kPlaylistItemIDKey, info.id);
  playlist_value.SetStringKey(kPlaylistItemTitleKey, info.title);
  playlist_value.SetStringKey(kPlaylistItemThumbnailPathKey,
                              info.thumbnail_path);
  playlist_value.SetStringKey(kPlaylistItemMediaFilePathKey,
                              info.media_file_path);
  playlist_value.SetBoolKey(kPlaylistItemReadyKey, info.ready);
  return playlist_value;
}

}  // namespace playlist
