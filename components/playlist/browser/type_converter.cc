/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/browser/type_converter.h"

#include <iterator>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/contains.h"
#include "base/json/values_util.h"
#include "brave/components/playlist/browser/playlist_constants.h"
#include "brave/components/playlist/browser/pref_names.h"
#include "brave/components/sync/protocol/playlist_specifics.pb.h"

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
constexpr char kPlaylistItemMediaFileCachedKey[] = "mediaCached";
constexpr char kPlaylistItemTitleKey[] = "title";
constexpr char kPlaylistItemAuthorKey[] = "author";
constexpr char kPlaylistItemDurationKey[] = "duration";
constexpr char kPlaylistItemLastPlayedPositionKey[] = "lastPlayedPosition";
constexpr char kPlaylistItemParentKey[] = "parent";
constexpr char kPlaylistItemMediaFileBytesKey[] = "mediaFileBytes";

}  // namespace

bool IsItemValueMalformed(const base::Value::Dict& dict) {
  return !dict.contains(kPlaylistItemIDKey) ||
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
  // DO NOT ADD MORE
}

void MigratePlaylistOrder(const base::Value::Dict& playlists,
                          base::Value::List& order) {
  base::flat_set<std::string> missing_ids;
  for (const auto [id, _] : playlists) {
    missing_ids.insert(id);
  }

  for (const auto& existing_id_value : order) {
    missing_ids.erase(existing_id_value.GetString());
  }

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

  base::Value::List parent;
  for (const auto& parent_playlist_id : item->parents) {
    parent.Append(base::Value(parent_playlist_id));
  }

  playlist_value.Set(kPlaylistItemParentKey, std::move(parent));

  return playlist_value;
}

mojom::PlaylistItemPtr ConvertPBToPlaylistItem(
    const sync_pb::PlaylistItemDetails& item_pb,
    const base::Value::Dict& items_cache_dict) {
  auto item = mojom::PlaylistItem::New();

  auto* item_cache = items_cache_dict.FindDict(item_pb.id());
  if (item_cache) {
    auto* thumbnail_path = item_cache->FindString(kPlaylistCacheThumbnailPathKey);
    item->thumbnail_path = GURL(thumbnail_path ? *thumbnail_path : "");
    auto* media_path = item_cache->FindString(kPlaylistCacheMediaPathKey);
    item->media_path = GURL(media_path ? *media_path : "");
    auto media_file_bytes = item_cache->FindDouble(kPlaylistCacheMediaPathKey);
    item->media_file_bytes = static_cast<uint64_t>(media_file_bytes.value_or(0));
    item->cached = item_cache->FindBool(kPlaylistCacheCachedKey).value_or(false);
  }  
  item->id = item_pb.id();
  item->name = item_pb.name();
  item->page_source = GURL(item_pb.page_source());
  item->thumbnail_source = GURL(item_pb.thumbnail_source());
  item->media_source = GURL(item_pb.media_source());
  item->duration = item_pb.duration();
  item->author = item_pb.author();
  item->last_played_position = item_pb.last_played_position();

  const auto& parents = item_pb.playlist_ids();
  std::copy(parents.cbegin(), parents.cend(), std::back_inserter(item->parents));

  return item;
}

sync_pb::PlaylistItemDetails ConvertPlaylistItemToPB(
    const mojom::PlaylistItemPtr& item) {
  sync_pb::PlaylistItemDetails result;

  result.set_id(item->id);
  result.set_name(item->name);
  result.set_page_source(item->page_source.spec());
  result.set_media_source(item->media_source.spec());
  result.set_thumbnail_source(item->thumbnail_source.spec());
  result.set_author(item->author);
  result.set_duration(item->duration);
  result.set_last_played_position(item->last_played_position);
  
  auto* result_playlist_ids = result.mutable_playlist_ids();
  for (const auto& parent : item->parents) {
    result_playlist_ids->Add(std::string(parent));
  }

  return result;
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

mojom::PlaylistPtr ConvertPBToPlaylist(
    const sync_pb::PlaylistDetails& playlist_pb,
    const std::vector<sync_pb::PlaylistItemDetails>& items_pb,
    const base::Value::Dict& items_cache_dict) {
  mojom::PlaylistPtr playlist = mojom::Playlist::New();

  playlist->id = playlist_pb.id();
  playlist->name = playlist_pb.name();

  std::transform(items_pb.cbegin(), items_pb.cend(), std::back_inserter(playlist->items), [&items_cache_dict](const auto& item_pb) {
    return ConvertPBToPlaylistItem(item_pb, items_cache_dict);
  });

  return playlist;
}

sync_pb::PlaylistDetails ConvertPlaylistToPB(const mojom::PlaylistPtr& playlist) {
  sync_pb::PlaylistDetails result;

  result.set_id(playlist->id.value_or(std::string()));
  result.set_name(playlist->name);

  auto* result_item_ids = result.mutable_playlist_item_ids();
  for (const auto& item : playlist->items) {
    result_item_ids->Add(std::string(item->id));
  }

  return result;
}

}  // namespace playlist
