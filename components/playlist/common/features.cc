/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/common/features.h"

#include "base/dcheck_is_on.h"
#include "base/feature_list.h"

namespace playlist::features {

BASE_FEATURE(kPlaylist, "Playlist", base::FEATURE_ENABLED_BY_DEFAULT);

BASE_FEATURE(kPlaylistFakeUA,
             "PlaylistFakeUA",
             base::FEATURE_ENABLED_BY_DEFAULT);

}  // namespace playlist::features
