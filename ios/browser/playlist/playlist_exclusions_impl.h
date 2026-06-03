// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_PLAYLIST_PLAYLIST_EXCLUSIONS_IMPL_H_
#define BRAVE_IOS_BROWSER_PLAYLIST_PLAYLIST_EXCLUSIONS_IMPL_H_

#include "brave/ios/browser/playlist/playlist_exclusions.h"

namespace playlist {
class PlaylistExclusions;
}

NS_ASSUME_NONNULL_BEGIN

@interface PlaylistExclusionsImpl : NSObject <PlaylistExclusionsBridge>

- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithPlaylistExclusions:
    (playlist::PlaylistExclusions&)playlistExclusions NS_DESIGNATED_INITIALIZER;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_PLAYLIST_PLAYLIST_EXCLUSIONS_IMPL_H_
