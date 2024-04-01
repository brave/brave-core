/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PLAYLIST_COMMON_PLAYLIST_RENDER_FRAME_OBSERVER_HELPER_H_
#define BRAVE_COMPONENTS_PLAYLIST_COMMON_PLAYLIST_RENDER_FRAME_OBSERVER_HELPER_H_

#include <vector>

#include "base/values.h"
#include "brave/components/playlist/common/mojom/playlist.mojom.h"

class GURL;

namespace playlist {

std::vector<mojom::PlaylistItemPtr> ExtractPlaylistItems(
    const GURL& url,
    base::Value::List list);

}  // namespace playlist

#endif  // BRAVE_COMPONENTS_PLAYLIST_COMMON_PLAYLIST_RENDER_FRAME_OBSERVER_HELPER_H_
