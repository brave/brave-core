/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/playlist_types.h"

#include "base/logging.h"
#include "base/notreached.h"

namespace playlist {

std::string PlaylistChangeParams::GetPlaylistChangeTypeAsString(
    PlaylistChangeParams::Type type) {
  switch (type) {
    case PlaylistChangeParams::Type::kItemAdded:
      return "item: added";
    case PlaylistChangeParams::Type::kItemDeleted:
      return "item: deleted";
    case PlaylistChangeParams::Type::kItemAborted:
      return "item: aborted";
    case PlaylistChangeParams::Type::kItemThumbnailReady:
      return "item: thumbnail_ready";
    case PlaylistChangeParams::Type::kItemThumbnailFailed:
      return "item: thumbnail_failed";
    case PlaylistChangeParams::Type::kItemPlayReady:
      return "item: play_ready";
    case PlaylistChangeParams::Type::kListCreated:
      return "list: created";
    case PlaylistChangeParams::Type::kAllDeleted:
      return "item: all deleted";
    case PlaylistChangeParams::Type::kNone:
      [[fallthrough]];
    default:
      NOTREACHED() << "Unknown type: " << static_cast<int>(type);
      return "item: unknown";
  }
}

PlaylistChangeParams::PlaylistChangeParams() = default;

PlaylistChangeParams::PlaylistChangeParams(Type type, const std::string& id)
    : change_type(type), playlist_id(id) {}
PlaylistChangeParams::~PlaylistChangeParams() = default;

PlaylistInfo::PlaylistInfo() = default;
PlaylistInfo::~PlaylistInfo() = default;
PlaylistInfo::PlaylistInfo(const PlaylistInfo& rhs) = default;
PlaylistInfo& PlaylistInfo::operator=(const PlaylistInfo& rhs) = default;
PlaylistInfo::PlaylistInfo(PlaylistInfo&& rhs) noexcept = default;
PlaylistInfo& PlaylistInfo::operator=(PlaylistInfo&& rhs) noexcept = default;

PlaylistItemInfo::PlaylistItemInfo() = default;
PlaylistItemInfo::PlaylistItemInfo(const Title& title,
                                   const ThumbnailPath& thumbnail_path,
                                   const MediaFilePath& media_file_path)
    : title(title),
      thumbnail_path(thumbnail_path),
      media_file_path(media_file_path) {}

PlaylistItemInfo::~PlaylistItemInfo() = default;
PlaylistItemInfo::PlaylistItemInfo(const PlaylistItemInfo& rhs) = default;
PlaylistItemInfo& PlaylistItemInfo::operator=(const PlaylistItemInfo& rhs) =
    default;
PlaylistItemInfo::PlaylistItemInfo(PlaylistItemInfo&& rhs) noexcept = default;
PlaylistItemInfo& PlaylistItemInfo::operator=(PlaylistItemInfo&& rhs) noexcept =
    default;

}  // namespace playlist
