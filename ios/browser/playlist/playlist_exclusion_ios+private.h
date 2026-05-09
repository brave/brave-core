// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_PLAYLIST_PLAYLIST_EXCLUSION_IOS_PRIVATE_H_
#define BRAVE_IOS_BROWSER_PLAYLIST_PLAYLIST_EXCLUSION_IOS_PRIVATE_H_

#include "brave/components/playlist/core/browser/playlist_exclusion.h"
#include "brave/ios/browser/playlist/playlist_exclusion_ios.h"

@interface PlaylistExclusionsAPIImpl : NSObject <PlaylistExclusionsAPI>
- (instancetype)initWithPlaylistExclusions:
    (playlist::PlaylistExclusions*)playlistExclusions;
- (instancetype)init NS_UNAVAILABLE;
@end

#endif  // BRAVE_IOS_BROWSER_PLAYLIST_PLAYLIST_EXCLUSION_IOS_PRIVATE_H_
