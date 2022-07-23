/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/playlist_types.h"

#include "base/logging.h"
#include "base/notreached.h"

namespace playlist {

std::string PlaylistItemChangeParams::GetPlaylistChangeTypeAsString(
    PlaylistItemChangeParams::Type type) {
  switch (type) {
    case PlaylistItemChangeParams::Type::kAdded:
      return "item: added";
    case PlaylistItemChangeParams::Type::kDeleted:
      return "item: deleted";
    case PlaylistItemChangeParams::Type::kAborted:
      return "item: aborted";
    case PlaylistItemChangeParams::Type::kThumbnailReady:
      return "item: thumbnail_ready";
    case PlaylistItemChangeParams::Type::kThumbnailFailed:
      return "item: thumbnail_failed";
    case PlaylistItemChangeParams::Type::kPlayReady:
      return "item: play_ready";
    case PlaylistItemChangeParams::Type::kAllDeleted:
      return "item: all deleted";
    case PlaylistItemChangeParams::Type::kNone:
      [[fallthrough]];
    default:
      NOTREACHED();
      return "item: unknown";
  }
}

PlaylistItemChangeParams::PlaylistItemChangeParams() = default;

PlaylistItemChangeParams::PlaylistItemChangeParams(Type type,
                                                   const std::string& id)
    : change_type(type), playlist_id(id) {}
PlaylistItemChangeParams::~PlaylistItemChangeParams() = default;

PlaylistItemInfo::PlaylistItemInfo() = default;
PlaylistItemInfo::~PlaylistItemInfo() = default;
PlaylistItemInfo::PlaylistItemInfo(const PlaylistItemInfo& rhs) = default;
PlaylistItemInfo& PlaylistItemInfo::operator=(const PlaylistItemInfo& rhs) =
    default;
PlaylistItemInfo::PlaylistItemInfo(PlaylistItemInfo&& rhs) noexcept = default;
PlaylistItemInfo& PlaylistItemInfo::operator=(PlaylistItemInfo&& rhs) noexcept =
    default;

}  // namespace playlist
