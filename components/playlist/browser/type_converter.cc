/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/browser/type_converter.h"

#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/contains.h"
#include "base/json/values_util.h"
#include "brave/components/playlist/browser/playlist_constants.h"

static_assert(
    std::numeric_limits<
        decltype(playlist::mojom::PlaylistItem::last_played_position)>::max() <=
        std::numeric_limits<int>::max(),
    "PlaylistItem's last_played_position could be overflowed when creating "
    "base::Value(int)");

namespace playlist {

namespace {

// Keys for Playlist's base::Dict --------------------------------------------
constexpr char kPlaylistIDKey[] = "id";
constexpr char kPlaylistNameKey[] = "name";
constexpr char kPlaylistItemsKey[] = "items";

// Keys for PlaylistItem's base::Dict ----------------------------------------
constexpr char kPlaylistItemIDKey[] = "id";
constexpr char kPlaylistItemPageSrcKey[] = "pageSrc";
constexpr char kPlaylistItemMediaSrcKey[] = "mediaSrc";
constexpr char kPlaylistItemThumbnailSrcKey[] = "thumbnailSrc";
constexpr char kPlaylistItemThumbnailPathKey[] = "thumbnailPath";
constexpr char kPlaylistItemMediaFilePathKey[] = "mediaFilePath";
#if BUILDFLAG(IS_ANDROID)
constexpr char kPlaylistItemHlsMediaFilePathKey[] = "hlsMediaFilePath";
#endif  // BUILDFLAG(IS_ANDROID)
constexpr char kPlaylistItemMediaFileCachedKey[] = "mediaCached";
constexpr char kPlaylistItemTitleKey[] = "title";
constexpr char kPlaylistItemAuthorKey[] = "author";
constexpr char kPlaylistItemDurationKey[] = "duration";
constexpr char kPlaylistItemLastPlayedPositionKey[] = "lastPlayedPosition";
constexpr char kPlaylistItemParentKey[] = "parent";
constexpr char kPlaylistItemMediaFileBytesKey[] = "mediaFileBytes";

#if BUILDFLAG(IS_ANDROID)
// Keys for HlsContent's base:Dict
constexpr char kHlsContentPlaylistItemIDKey[] = "playlistItemId";
constexpr char kIsPreparedKey[] = "isPrepared";
#endif  // BUILDFLAG(IS_ANDROID)

}  // namespace

bool IsItemValueMalformed(const base::Value::Dict& dict) {
  bool isMalformed = !dict.contains(kPlaylistItemIDKey) ||
                     !dict.contains(kPlaylistItemTitleKey) ||
                     !dict.contains(kPlaylistItemThumbnailPathKey) ||
                     !dict.contains(kPlaylistItemMediaFileCachedKey) ||
                     // Added 2022. Sep
                     !dict.contains(kPlaylistItemPageSrcKey) ||
                     !dict.contains(kPlaylistItemMediaSrcKey) ||
                     !dict.contains(kPlaylistItemThumbnailSrcKey) ||
                     !dict.contains(kPlaylistItemMediaFilePathKey) ||
                     // Added 2022. Dec.
                     !dict.contains(kPlaylistItemDurationKey) ||
                     !dict.contains(kPlaylistItemAuthorKey) ||
                     !dict.contains(kPlaylistItemLastPlayedPositionKey) ||
                     // Added 2023. Jan.
                     !dict.contains(kPlaylistItemParentKey) ||
                     // Added 2023. Aug.
                     !dict.contains(kPlaylistItemMediaFileBytesKey);

#if BUILDFLAG(IS_ANDROID)
  // Added 2023 Dec.
  isMalformed = isMalformed || !dict.contains(kPlaylistItemHlsMediaFilePathKey);
#endif  // BUILDFLAG(IS_ANDROID)
  return isMalformed;
  // DO NOT ADD MORE
}

bool IsHlsContentValueMalformed(const base::Value::Dict& dict) {
  bool isMalformed = !dict.contains(kHlsContentPlaylistItemIDKey) ||
                     !dict.contains(kIsPreparedKey);
  return isMalformed;
  // DO NOT ADD MORE
}

void MigratePlaylistOrder(const base::Value::Dict& playlists,
                          base::Value::List& order) {
  base::flat_set<std::string> missing_ids;
  for (const auto [id, _] : playlists) {
    missing_ids.insert(id);
  }

  base::flat_set<std::string> removed_ids;
  for (const auto& existing_id_value : order) {
    auto existing_id = existing_id_value.GetString();
    if (base::Contains(missing_ids, existing_id)) {
      missing_ids.erase(existing_id);
    } else {
      removed_ids.insert(existing_id);
    }
  }

  // Added 2024.01.
  // Data resetting had left dangled data in the order list and it caused crash
  order.EraseIf([&](const auto& id_value) {
    return base::Contains(removed_ids, id_value.GetString());
  });

  for (const auto& id : missing_ids) {
    order.Append(id);
  }
}

mojom::PlaylistItemPtr ConvertValueToPlaylistItem(
    const base::Value::Dict& dict) {
  DCHECK(!IsItemValueMalformed(dict));

  auto item = mojom::PlaylistItem::New();
  item->id = *dict.FindString(kPlaylistItemIDKey);
  item->name = *dict.FindString(kPlaylistItemTitleKey);
  item->page_source = GURL(*dict.FindString(kPlaylistItemPageSrcKey));
  item->thumbnail_source = GURL(*dict.FindString(kPlaylistItemThumbnailSrcKey));
  item->thumbnail_path = GURL(*dict.FindString(kPlaylistItemThumbnailPathKey));
  item->media_source = GURL(*dict.FindString(kPlaylistItemMediaSrcKey));
  item->media_path = GURL(*dict.FindString(kPlaylistItemMediaFilePathKey));
#if BUILDFLAG(IS_ANDROID)
  item->hls_media_path =
      GURL(*dict.FindString(kPlaylistItemHlsMediaFilePathKey));
#endif  // BUILDFLAG(IS_ANDROID)
  item->cached = *dict.FindBool(kPlaylistItemMediaFileCachedKey);
  item->duration = *dict.FindString(kPlaylistItemDurationKey);
  item->author = *dict.FindString(kPlaylistItemAuthorKey);
  item->last_played_position =
      *dict.FindInt(kPlaylistItemLastPlayedPositionKey);
  item->media_file_bytes = *dict.FindDouble(kPlaylistItemMediaFileBytesKey);

  const auto* parents = dict.FindList(kPlaylistItemParentKey);
  for (const auto& id_value : *parents) {
    DCHECK(id_value.is_string());
    const auto& id = id_value.GetString();
    DCHECK(!id.empty());
    item->parents.push_back(id);
  }

  return item;
}

base::Value::Dict ConvertPlaylistItemToValue(
    const mojom::PlaylistItemPtr& item) {
  base::Value::Dict playlist_value =
      base::Value::Dict()
          .Set(kPlaylistItemIDKey, item->id)
          .Set(kPlaylistItemTitleKey, item->name)
          .Set(kPlaylistItemPageSrcKey, item->page_source.spec())
          .Set(kPlaylistItemMediaSrcKey, item->media_source.spec())
          .Set(kPlaylistItemThumbnailSrcKey, item->thumbnail_source.spec())
          .Set(kPlaylistItemThumbnailPathKey, item->thumbnail_path.spec())
          .Set(kPlaylistItemMediaFilePathKey, item->media_path.spec())
          .Set(kPlaylistItemMediaFileCachedKey, item->cached)
          .Set(kPlaylistItemAuthorKey, item->author)
          .Set(kPlaylistItemDurationKey, item->duration)
          .Set(kPlaylistItemLastPlayedPositionKey, item->last_played_position)
          .Set(kPlaylistItemMediaFileBytesKey,
               static_cast<double>(item->media_file_bytes));
#if BUILDFLAG(IS_ANDROID)
  playlist_value.Set(kPlaylistItemHlsMediaFilePathKey,
                     item->hls_media_path.spec());
#endif  // BUILDFLAG(IS_ANDROID)

  base::Value::List parent;
  for (const auto& parent_playlist_id : item->parents) {
    parent.Append(base::Value(parent_playlist_id));
  }

  playlist_value.Set(kPlaylistItemParentKey, std::move(parent));

  return playlist_value;
}

mojom::PlaylistPtr ConvertValueToPlaylist(
    const base::Value::Dict& playlist_dict,
    const base::Value::Dict& items_dict) {
  mojom::PlaylistPtr playlist = mojom::Playlist::New();
  playlist->id = *playlist_dict.FindString(kPlaylistIDKey);
  playlist->name = *playlist_dict.FindString(kPlaylistNameKey);
  for (const auto& item_id_value : *playlist_dict.FindList(kPlaylistItemsKey)) {
    auto* item = items_dict.FindDict(item_id_value.GetString());
    DCHECK(item) << "Couldn't find PlaylistItem with id: "
                 << item_id_value.GetString();
    playlist->items.push_back(ConvertValueToPlaylistItem(*item));
  }
  return playlist;
}

base::Value::Dict ConvertPlaylistToValue(const mojom::PlaylistPtr& playlist) {
  base::Value::Dict value;
  value.Set(kPlaylistIDKey, playlist->id.value());
  value.Set(kPlaylistNameKey, playlist->name);
  auto item_ids = base::Value::List();
  for (const auto& items : playlist->items) {
    item_ids.Append(items->id);
  }
  value.Set(kPlaylistItemsKey, std::move(item_ids));
  return value;
}

#if BUILDFLAG(IS_ANDROID)
mojom::HlsContentPtr ConvertValueToHlsContent(const base::Value::Dict& dict) {
  DCHECK(!IsHlsContentValueMalformed(dict));

  auto hls_content = mojom::HlsContent::New();
  hls_content->playlist_item_id =
      *dict.FindString(kHlsContentPlaylistItemIDKey);
  hls_content->is_prepared = *dict.FindBool(kIsPreparedKey);
  return hls_content;
}

base::Value::Dict ConvertHlsContentToValue(
    const mojom::HlsContentPtr& hls_content) {
  base::Value::Dict hls_content_value =
      base::Value::Dict()
          .Set(kHlsContentPlaylistItemIDKey, hls_content->playlist_item_id)
          .Set(kIsPreparedKey, hls_content->is_prepared);

  return hls_content_value;
}
#endif  // BUILDFLAG(IS_ANDROID)

}  // namespace playlist
