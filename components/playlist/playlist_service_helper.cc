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

bool IsItemValueMalformed(const base::Value::Dict& dict) {
  return !dict.contains(playlist::kPlaylistItemIDKey) ||
         !dict.contains(playlist::kPlaylistItemTitleKey) ||
         !dict.contains(playlist::kPlaylistItemThumbnailPathKey) ||
         !dict.contains(playlist::kPlaylistItemMediaFileCachedKey) ||
         // Added 2022. Sep
         !dict.contains(playlist::kPlaylistItemPageSrcKey) ||
         !dict.contains(playlist::kPlaylistItemMediaSrcKey) ||
         !dict.contains(playlist::kPlaylistItemThumbnailSrcKey) ||
         !dict.contains(playlist::kPlaylistItemMediaFilePathKey) ||
         // Added 2022, dec.
         !dict.contains(playlist::kPlaylistItemDurationKey) ||
         !dict.contains(playlist::kPlaylistItemAuthorKey) ||
         !dict.contains(playlist::kPlaylistItemLastPlayedPositionKey);
}

mojom::PlaylistItemPtr ConvertValueToPlaylistItem(
    const base::Value::Dict& dict) {
  DCHECK(!IsItemValueMalformed(dict));

  auto item = mojom::PlaylistItem::New();
  item->id = *dict.FindString(playlist::kPlaylistItemIDKey);
  item->name = *dict.FindString(playlist::kPlaylistItemTitleKey);
  item->page_source = GURL(*dict.FindString(playlist::kPlaylistItemPageSrcKey));
  item->thumbnail_source =
      GURL(*dict.FindString(playlist::kPlaylistItemThumbnailSrcKey));
  item->thumbnail_path =
      GURL(*dict.FindString(playlist::kPlaylistItemThumbnailPathKey));
  item->media_source =
      GURL(*dict.FindString(playlist::kPlaylistItemMediaSrcKey));
  item->media_path =
      GURL(*dict.FindString(playlist::kPlaylistItemMediaFilePathKey));
  item->cached = *dict.FindBool(playlist::kPlaylistItemMediaFileCachedKey);
  item->duration = *dict.FindString(playlist::kPlaylistItemDurationKey);
  item->author = *dict.FindString(playlist::kPlaylistItemAuthorKey);
  item->last_played_position =
      *dict.FindInt(playlist::kPlaylistItemLastPlayedPositionKey);
  return item;
}

base::Value::Dict ConvertPlaylistItemToValue(
    const mojom::PlaylistItemPtr& item) {
  base::Value::Dict playlist_value;
  playlist_value.Set(kPlaylistItemIDKey, item->id);
  playlist_value.Set(kPlaylistItemTitleKey, item->name);
  playlist_value.Set(kPlaylistItemPageSrcKey, item->page_source.spec());
  playlist_value.Set(kPlaylistItemMediaSrcKey, item->media_source.spec());
  playlist_value.Set(kPlaylistItemThumbnailSrcKey,
                     item->thumbnail_source.spec());
  playlist_value.Set(kPlaylistItemThumbnailPathKey,
                     item->thumbnail_path.spec());
  playlist_value.Set(kPlaylistItemMediaFilePathKey, item->media_path.spec());
  playlist_value.Set(kPlaylistItemMediaFileCachedKey, item->cached);
  playlist_value.Set(kPlaylistItemAuthorKey, item->author);
  playlist_value.Set(kPlaylistItemDurationKey, item->duration);
  playlist_value.Set(kPlaylistItemLastPlayedPositionKey,
                     item->last_played_position);
  return playlist_value;
}

}  // namespace playlist
