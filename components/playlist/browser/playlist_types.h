/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PLAYLIST_BROWSER_PLAYLIST_TYPES_H_
#define BRAVE_COMPONENTS_PLAYLIST_BROWSER_PLAYLIST_TYPES_H_

#include <string>
#include <vector>

#include "base/time/time.h"
#include "base/types/strong_alias.h"

#include "brave/components/playlist/common/mojom/playlist.mojom.h"

namespace playlist {

// TODO(sko) Try removing these types. We can use mojom type directly.
// https://github.com/brave/brave-browser/issues/27516
struct PlaylistChangeParams {
  static std::string GetPlaylistChangeTypeAsString(mojom::PlaylistEvent type);

  PlaylistChangeParams();
  PlaylistChangeParams(mojom::PlaylistEvent type, const std::string& id);
  ~PlaylistChangeParams();

  mojom::PlaylistEvent change_type = mojom::PlaylistEvent::kNone;
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

#endif  // BRAVE_COMPONENTS_PLAYLIST_BROWSER_PLAYLIST_TYPES_H_
