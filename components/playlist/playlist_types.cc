/* Copyright (c) 2021 The Brave Authors. All rights reserved.
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
      [[fallthrough]];
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
CreatePlaylistParams::CreatePlaylistParams(const CreatePlaylistParams& rhs) =
    default;
CreatePlaylistParams& CreatePlaylistParams::operator=(
    const CreatePlaylistParams& rhs) = default;
CreatePlaylistParams::CreatePlaylistParams(
    CreatePlaylistParams&& rhs) noexcept = default;
CreatePlaylistParams& CreatePlaylistParams::operator=(
    CreatePlaylistParams&& rhs) noexcept = default;

PlaylistInfo::PlaylistInfo() = default;
PlaylistInfo::~PlaylistInfo() = default;
PlaylistInfo::PlaylistInfo(const PlaylistInfo& rhs) = default;
PlaylistInfo& PlaylistInfo::operator=(const PlaylistInfo& rhs) = default;
PlaylistInfo::PlaylistInfo(PlaylistInfo&& rhs) noexcept = default;
PlaylistInfo& PlaylistInfo::operator=(PlaylistInfo&& rhs) noexcept = default;

}  // namespace playlist
