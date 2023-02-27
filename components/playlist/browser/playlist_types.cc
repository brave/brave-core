/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/browser/playlist_types.h"

#include "base/logging.h"
#include "base/notreached.h"

namespace playlist {

std::string PlaylistChangeParams::GetPlaylistChangeTypeAsString(
    mojom::PlaylistEvent type) {
  switch (type) {
    case mojom::PlaylistEvent::kItemAdded:
      return "item: added";
    case mojom::PlaylistEvent::kItemDeleted:
      return "item: deleted";
    case mojom::PlaylistEvent::kItemAborted:
      return "item: aborted";
    case mojom::PlaylistEvent::kItemThumbnailReady:
      return "item: thumbnail_ready";
    case mojom::PlaylistEvent::kItemThumbnailFailed:
      return "item: thumbnail_failed";
    case mojom::PlaylistEvent::kItemCached:
      return "item: cached";
    case mojom::PlaylistEvent::kListCreated:
      return "list: created";
    case mojom::PlaylistEvent::kAllDeleted:
      return "item: all deleted";
    case mojom::PlaylistEvent::kNone:
      [[fallthrough]];
    default:
      NOTREACHED() << "Unknown type: " << static_cast<int>(type);
      return "item: unknown";
  }
}

PlaylistChangeParams::PlaylistChangeParams() = default;

PlaylistChangeParams::PlaylistChangeParams(mojom::PlaylistEvent type,
                                           const std::string& id)
    : change_type(type), playlist_id(id) {}
PlaylistChangeParams::~PlaylistChangeParams() = default;

bool PlaylistChangeParams::operator==(const PlaylistChangeParams& rhs) const {
  return change_type == rhs.change_type && playlist_id == rhs.playlist_id;
}

}  // namespace playlist
