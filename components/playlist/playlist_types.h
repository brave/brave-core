/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PLAYLIST_PLAYLIST_TYPES_H_
#define BRAVE_COMPONENTS_PLAYLIST_PLAYLIST_TYPES_H_

#include <string>
#include <vector>

namespace playlist {

struct PlaylistChangeParams {
  enum class Type {
    kNone,
    kItemAdded,            // a new playlist item added but not ready state
    kItemThumbnailReady,   // Thumbnail ready to use for playlist
    kItemThumbnailFailed,  // Failed to fetch thumbnail
    kItemPlayReady,        // Playlist ready to play
    kItemDeleted,          // A playlist deleted
    kItemAborted,          // Aborted during the creation process

    kListCreated,  // A list is created
    kListRemoved,  // A list is removed
    kAllDeleted,   // All playlist are deleted
  };
  static std::string GetPlaylistChangeTypeAsString(Type type);

  PlaylistChangeParams();
  PlaylistChangeParams(Type type, const std::string& id);
  ~PlaylistChangeParams();

  Type change_type = Type::kNone;
  std::string playlist_id;
};

struct PlaylistItemInfo {
  PlaylistItemInfo();
  PlaylistItemInfo(const PlaylistItemInfo& rhs);
  PlaylistItemInfo& operator=(const PlaylistItemInfo& rhs);
  PlaylistItemInfo(PlaylistItemInfo&& rhs) noexcept;
  PlaylistItemInfo& operator=(PlaylistItemInfo&& rhs) noexcept;
  ~PlaylistItemInfo();

  std::string id;
  std::string title;
  std::string thumbnail_path;
  std::string media_file_path;
  bool ready{false};
};

struct PlaylistInfo {
  PlaylistInfo();
  PlaylistInfo(const PlaylistInfo& rhs);
  PlaylistInfo& operator=(const PlaylistInfo& rhs);
  PlaylistInfo(PlaylistInfo&& rhs) noexcept;
  PlaylistInfo& operator=(PlaylistInfo&& rhs) noexcept;
  ~PlaylistInfo();

  std::string id;
  std::string name;
  std::vector<PlaylistItemInfo> items;
};

}  // namespace playlist

#endif  // BRAVE_COMPONENTS_PLAYLIST_PLAYLIST_TYPES_H_
