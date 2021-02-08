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
  enum class ChangeType {
    kChangeTypeNone,
    kChangeTypeAdded,            // New playlist added but not ready state
    kChangeTypeThumbnailReady,   // Thumbnail ready to use for playlist
    kChangeTypeThumbnailFailed,  // Failed to fetch thumbnail
    kChangeTypePlayReady,        // Playlist ready to play
    kChangeTypeDeleted,          // A playlist deleted
    kChangeTypeAllDeleted,       // All playlist are deleted
    kChangeTypeAborted,          // Aborted during the creation process
  };
  static std::string GetPlaylistChangeTypeAsString(
      PlaylistChangeParams::ChangeType type);

  PlaylistChangeParams();
  PlaylistChangeParams(ChangeType type, const std::string& id);
  ~PlaylistChangeParams();

  ChangeType change_type = ChangeType::kChangeTypeNone;
  std::string playlist_id;
};

struct MediaFileInfo {
  MediaFileInfo(const std::string& url, const std::string& title);
  ~MediaFileInfo();

  std::string media_file_url;
  std::string media_file_title;
};

struct CreatePlaylistParams {
  CreatePlaylistParams();
  CreatePlaylistParams(const CreatePlaylistParams& rhs);
  ~CreatePlaylistParams();

  std::string playlist_thumbnail_url;
  std::string playlist_name;
  std::vector<MediaFileInfo> video_media_files;
  std::vector<MediaFileInfo> audio_media_files;
};

// TODO(simonhong): Rename to PlaylistItemInfo
struct PlaylistInfo {
  PlaylistInfo();
  PlaylistInfo(const PlaylistInfo& rhs);
  ~PlaylistInfo();

  std::string id;
  // TODO(simonhong): Delete this. |create_params| has it.
  std::string playlist_name;
  std::string thumbnail_path;
  std::string video_media_file_path;
  std::string audio_media_file_path;
  bool ready{false};

  CreatePlaylistParams create_params;
};

}  // namespace playlist

#endif  // BRAVE_COMPONENTS_PLAYLIST_PLAYLIST_TYPES_H_
