/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/playlists/browser/playlists_types.h"

#include <base/logging.h>

namespace brave_playlists {

std::string PlaylistsChangeParams::GetPlaylistsChangeTypeAsString(
    PlaylistsChangeParams::ChangeType type) {
  switch (type) {
    case PlaylistsChangeParams::ChangeType::CHANGE_TYPE_ADDED:
      return "added";
    case PlaylistsChangeParams::ChangeType::CHANGE_TYPE_DELETED:
      return "deleted";
    case PlaylistsChangeParams::ChangeType::CHANGE_TYPE_ALL_DELETED:
      return "all_deleted";
    case PlaylistsChangeParams::ChangeType::CHANGE_TYPE_ABORTED:
      return "aborted";
    case PlaylistsChangeParams::ChangeType::CHANGE_TYPE_THUMBNAIL_READY:
      return "thumbnail_ready";
    case PlaylistsChangeParams::ChangeType::CHANGE_TYPE_THUMBNAIL_FAILED:
      return "thumbnail_failed";
    case PlaylistsChangeParams::ChangeType::CHANGE_TYPE_PLAY_READY:
      return "play_ready";
    case PlaylistsChangeParams::ChangeType::CHANGE_TYPE_PLAY_READY_PARTIAL:
      return "play_ready_partial";
    case PlaylistsChangeParams::ChangeType::CHANGE_TYPE_NONE:  // fall through
    default:
      NOTREACHED();
      return "unknown";
  }
}

PlaylistsChangeParams::PlaylistsChangeParams() {}

PlaylistsChangeParams::PlaylistsChangeParams(ChangeType type,
                                             const std::string& id)
    : change_type(type), playlist_id(id) {}
PlaylistsChangeParams::~PlaylistsChangeParams() {}

MediaFileInfo::MediaFileInfo(const std::string& url, const std::string& title)
    : media_file_url(url), media_file_title(title) {}
MediaFileInfo::~MediaFileInfo() {}

CreatePlaylistParams::CreatePlaylistParams() {}
CreatePlaylistParams::~CreatePlaylistParams() {}
CreatePlaylistParams::CreatePlaylistParams(const CreatePlaylistParams& rhs) {
  playlist_thumbnail_url = rhs.playlist_thumbnail_url;
  playlist_name = rhs.playlist_name;
  video_media_files = rhs.video_media_files;
  audio_media_files = rhs.audio_media_files;
}

PlaylistInfo::PlaylistInfo() {}
PlaylistInfo::~PlaylistInfo() {}
PlaylistInfo::PlaylistInfo(const PlaylistInfo& rhs) {
  id = rhs.id;
  playlist_name = rhs.playlist_name;
  thumbnail_path = rhs.thumbnail_path;
  video_media_file_path = rhs.video_media_file_path;
  audio_media_file_path = rhs.audio_media_file_path;
  create_params = rhs.create_params;
  partial_ready = rhs.partial_ready;
}

}  // namespace brave_playlists
