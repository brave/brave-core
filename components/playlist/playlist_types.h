/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PLAYLIST_PLAYLIST_TYPES_H_
#define BRAVE_COMPONENTS_PLAYLIST_PLAYLIST_TYPES_H_

#include <string>
#include <vector>

#include "base/time/time.h"
#include "base/types/strong_alias.h"

namespace playlist {

// TODO(sko) Try removing these types. We can use mojom type directly.
// https://github.com/brave/brave-browser/issues/27516
struct PlaylistChangeParams {
  enum class Type {
    kNone,
    kItemAdded,             // a new playlist item added but not ready state
    kItemThumbnailReady,    // Thumbnail ready to use for playlist
    kItemThumbnailFailed,   // Failed to fetch thumbnail
    kItemCached,            // The item is cached in local storage
    kItemDeleted,           // An item deleted
    kItemUpdated,           // An item's properties have been changed
    kItemAborted,           // Aborted during the creation process
    kItemLocalDataRemoved,  // Local data removed

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

  bool operator==(const PlaylistChangeParams& rhs) const;

  friend std::ostream& operator<<(std::ostream& os,
                                  const PlaylistChangeParams& change_params) {
    return os << "{ "
              << GetPlaylistChangeTypeAsString(change_params.change_type)
              << " }";
  }
};

}  // namespace playlist

#endif  // BRAVE_COMPONENTS_PLAYLIST_PLAYLIST_TYPES_H_
