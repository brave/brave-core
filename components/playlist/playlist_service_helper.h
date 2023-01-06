/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PLAYLIST_PLAYLIST_SERVICE_HELPER_H_
#define BRAVE_COMPONENTS_PLAYLIST_PLAYLIST_SERVICE_HELPER_H_

#include "base/values.h"
#include "brave/components/playlist/mojom/playlist.mojom.h"
#include "brave/components/playlist/playlist_types.h"

namespace playlist {

// This class encapsulates how we convert data between mojo types and
// base::Value. As base::Value's APIs are not that stable and quite confusing to
// use, we'd like to strictly limit where it's done.
class TypeConverter final {
 public:
  static bool IsItemValueMalformed(const base::Value::Dict& dict);

  // Converters between mojom::PlaylistItem and base::Value --------------------
  static mojom::PlaylistItemPtr ConvertValueToPlaylistItem(
      const base::Value::Dict& dict);
  static base::Value::Dict ConvertPlaylistItemToValue(
      const mojom::PlaylistItemPtr& item);

  // Converters between mojom::PlaylistItemList and base::Value ----------------
  // Note that Playlist value only contains the ids of its
  // children. The actual value of the children is stored in a separate value.
  // This is to make playlist items can be shared by multiple playlists. For
  // more details, please see a comment in playlist/pref_names.h
  static mojom::PlaylistPtr ConvertValueToPlaylist(
      const base::Value::Dict& playlist_dict,
      const base::Value::Dict& items_dict);
  static base::Value::Dict ConvertPlaylistToValue(
      const mojom::PlaylistPtr& playlist);

 private:
  // Don't allow instantiation.
  TypeConverter() = delete;
  ~TypeConverter() = delete;

  // Exposes keys to tests
  FRIEND_TEST_ALL_PREFIXES(PlaylistServiceUnitTest, AddItemsToList);
  FRIEND_TEST_ALL_PREFIXES(PlaylistServiceUnitTest, MediaDownloadFailed);
  FRIEND_TEST_ALL_PREFIXES(PlaylistServiceUnitTest, MediaRecoverTest);
  FRIEND_TEST_ALL_PREFIXES(PlaylistServiceUnitTest, MoveItem);

  // Keys for Playlist's base::Dict --------------------------------------------
  static constexpr char kPlaylistIDKey[] = "id";
  static constexpr char kPlaylistNameKey[] = "name";
  static constexpr char kPlaylistItemsKey[] = "items";

  // Keys for PlaylistItem's base::Dict ----------------------------------------
  static constexpr char kPlaylistItemIDKey[] = "id";
  static constexpr char kPlaylistItemPageSrcKey[] = "pageSrc";
  static constexpr char kPlaylistItemMediaSrcKey[] = "mediaSrc";
  static constexpr char kPlaylistItemThumbnailSrcKey[] = "thumbnailSrc";
  static constexpr char kPlaylistItemThumbnailPathKey[] = "thumbnailPath";
  static constexpr char kPlaylistItemMediaFilePathKey[] = "mediaFilePath";
  static constexpr char kPlaylistItemMediaFileCachedKey[] = "mediaCached";
  static constexpr char kPlaylistItemTitleKey[] = "title";
  static constexpr char kPlaylistItemAuthorKey[] = "author";
  static constexpr char kPlaylistItemDurationKey[] = "duration";
  static constexpr char kPlaylistItemLastPlayedPositionKey[] =
      "lastPlayedPosition";
};

}  // namespace playlist

#endif  // BRAVE_COMPONENTS_PLAYLIST_PLAYLIST_SERVICE_HELPER_H_
