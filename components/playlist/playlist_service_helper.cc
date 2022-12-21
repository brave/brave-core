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
  playlist_value.Set(kPlaylistItemLastPlayedPositionKey,
                     info.last_played_position);
  return playlist_value;
}

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

PlaylistItemInfo GetPlaylistItemInfoFromValue(const base::Value::Dict& dict) {
  DCHECK(!IsItemValueMalformed(dict));

  PlaylistItemInfo item;
  item.id = *dict.FindString(playlist::kPlaylistItemIDKey);
  item.title = *dict.FindString(playlist::kPlaylistItemTitleKey);
  item.page_src = *dict.FindString(playlist::kPlaylistItemPageSrcKey);
  item.thumbnail_src = *dict.FindString(playlist::kPlaylistItemThumbnailSrcKey);
  item.thumbnail_path =
      *dict.FindString(playlist::kPlaylistItemThumbnailPathKey);
  item.media_src = *dict.FindString(playlist::kPlaylistItemMediaSrcKey);
  item.media_file_path =
      *dict.FindString(playlist::kPlaylistItemMediaFilePathKey);
  item.media_file_cached =
      *dict.FindBool(playlist::kPlaylistItemMediaFileCachedKey);
  item.duration =
      base::ValueToTimeDelta(dict.Find(playlist::kPlaylistItemDurationKey))
          .value_or(base::TimeDelta());
  item.author = *dict.FindString(playlist::kPlaylistItemAuthorKey);
  item.last_played_position =
      *dict.FindInt(playlist::kPlaylistItemLastPlayedPositionKey);
  return item;
}

mojo::StructPtr<mojom::PlaylistItem> GetPlaylistItemMojoFromInfo(
    const PlaylistItemInfo& info) {
  return mojom::PlaylistItem::New(
      info.id, info.title, GURL(info.page_src), GURL(info.media_src),
      GURL(info.thumbnail_src), GURL(info.media_file_path),
      GURL(info.thumbnail_path), info.media_file_cached, info.author,
      base::TimeDeltaToValue(info.duration).GetString(),
      info.last_played_position);
}

PlaylistItemInfo GetPlaylistItemInfoFromMojo(const mojom::PlaylistItemPtr& mojo) {
  PlaylistItemInfo info;
  info.id = mojo->id;
  info.title = mojo->name;
  info.page_src = mojo->page_source.spec();
  info.media_src = mojo->media_source.spec();
  info.thumbnail_src = mojo->thumbnail_source.spec();
  info.media_file_path = mojo->media_path.spec();
  info.media_file_cached = mojo->cached;
  info.author = mojo->author;
  info.duration = base::ValueToTimeDelta(base::Value(mojo->duration))
                      .value_or(base::TimeDelta());
  info.last_played_position = mojo->last_played_position;
  return info;
}

}  // namespace playlist
