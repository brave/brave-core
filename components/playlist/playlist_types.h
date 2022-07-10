/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PLAYLIST_PLAYLIST_TYPES_H_
#define BRAVE_COMPONENTS_PLAYLIST_PLAYLIST_TYPES_H_

#include <string>
#include <vector>

namespace playlist {

struct PlaylistItemChangeParams {
  enum class Type {
    kNone,
    kAdded,            // New playlist added but not ready state
    kThumbnailReady,   // Thumbnail ready to use for playlist
    kThumbnailFailed,  // Failed to fetch thumbnail
    kPlayReady,        // Playlist ready to play
    kDeleted,          // A playlist deleted
    kAborted,          // Aborted during the creation process

    // TODO(sko) This should be event of Playlist, not of PlaylistItem.
    kAllDeleted,  // All playlist are deleted
  };
  static std::string GetPlaylistChangeTypeAsString(Type type);

  PlaylistItemChangeParams();
  PlaylistItemChangeParams(Type type, const std::string& id);
  ~PlaylistItemChangeParams();

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

}  // namespace playlist

#endif  // BRAVE_COMPONENTS_PLAYLIST_PLAYLIST_TYPES_H_
