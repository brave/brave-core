/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PLAYLIST_PLAYLIST_SERVICE_HELPER_H_
#define BRAVE_COMPONENTS_PLAYLIST_PLAYLIST_SERVICE_HELPER_H_

#include "base/values.h"
#include "brave/components/playlist/mojom/playlist.mojom.h"
#include "brave/components/playlist/playlist_types.h"

namespace playlist {

bool IsItemValueMalformed(const base::Value::Dict& dict);

mojom::PlaylistItemPtr ConvertValueToPlaylistItem(
    const base::Value::Dict& dict);

base::Value::Dict ConvertPlaylistItemToValue(
    const mojom::PlaylistItemPtr& item);

}  // namespace playlist

#endif  // BRAVE_COMPONENTS_PLAYLIST_PLAYLIST_SERVICE_HELPER_H_
