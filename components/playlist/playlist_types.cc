/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/playlist_types.h"

#include "base/logging.h"
#include "base/notreached.h"

namespace playlist {

std::string PlaylistChangeParams::GetPlaylistChangeTypeAsString(
    PlaylistChangeParams::ChangeType type) {
  switch (type) {
    case PlaylistChangeParams::ChangeType::kChangeTypeAdded:
      return "added";
    case PlaylistChangeParams::ChangeType::kChangeTypeDeleted:
      return "deleted";
    case PlaylistChangeParams::ChangeType::kChangeTypeAllDeleted:
      return "all_deleted";
    case PlaylistChangeParams::ChangeType::kChangeTypeAborted:
      return "aborted";
    case PlaylistChangeParams::ChangeType::kChangeTypeThumbnailReady:
      return "thumbnail_ready";
    case PlaylistChangeParams::ChangeType::kChangeTypeThumbnailFailed:
      return "thumbnail_failed";
    case PlaylistChangeParams::ChangeType::kChangeTypePlayReady:
      return "play_ready";
    case PlaylistChangeParams::ChangeType::kChangeTypeNone:
      FALLTHROUGH;
    default:
      NOTREACHED();
      return "unknown";
  }
}

PlaylistChangeParams::PlaylistChangeParams() = default;

PlaylistChangeParams::PlaylistChangeParams(ChangeType type,
                                           const std::string& id)
    : change_type(type), playlist_id(id) {}
PlaylistChangeParams::~PlaylistChangeParams() = default;

MediaFileInfo::MediaFileInfo(const std::string& url, const std::string& title)
    : media_file_url(url), media_file_title(title) {}
MediaFileInfo::~MediaFileInfo() {}

CreatePlaylistParams::CreatePlaylistParams() = default;
CreatePlaylistParams::~CreatePlaylistParams() = default;
CreatePlaylistParams::CreatePlaylistParams(const CreatePlaylistParams& rhs) {
  playlist_thumbnail_url = rhs.playlist_thumbnail_url;
  playlist_name = rhs.playlist_name;
  video_media_files = rhs.video_media_files;
  audio_media_files = rhs.audio_media_files;
}

PlaylistInfo::PlaylistInfo() = default;
PlaylistInfo::~PlaylistInfo() = default;
PlaylistInfo::PlaylistInfo(const PlaylistInfo& rhs) {
  id = rhs.id;
  playlist_name = rhs.playlist_name;
  thumbnail_path = rhs.thumbnail_path;
  video_media_file_path = rhs.video_media_file_path;
  audio_media_file_path = rhs.audio_media_file_path;
  create_params = rhs.create_params;
  ready = rhs.ready;
}

}  // namespace playlist
