// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/playlist/playlist_exclusions_factory.h"

#include "brave/components/playlist/core/browser/playlist_exclusion.h"
#include "brave/ios/browser/playlist/playlist_exclusions_impl.h"

@implementation PlaylistExclusionsFactory

+ (id<PlaylistExclusionsBridge>)sharedPlaylistExclusions {
  return [[PlaylistExclusionsImpl alloc]
      initWithPlaylistExclusions:*playlist::PlaylistExclusions::GetInstance()];
}

@end
