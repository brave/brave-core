// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_PLAYLIST_FEATURES_H_
#define BRAVE_IOS_BROWSER_PLAYLIST_FEATURES_H_

#import "base/feature_list.h"

namespace playlist::features {

BASE_DECLARE_FEATURE(kPlaylistOfflineCacheEnabled);
BASE_DECLARE_FEATURE(kPlaylistCacheFirstEnabled);

}  // namespace playlist::features

#endif  // BRAVE_IOS_BROWSER_PLAYLIST_FEATURES_H_
