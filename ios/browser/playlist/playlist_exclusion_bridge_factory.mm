// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/playlist/playlist_exclusion_bridge_factory.h"

#include "brave/components/playlist/core/browser/playlist_exclusion.h"
#include "brave/ios/browser/playlist/playlist_exclusion_bridge_impl.h"

@implementation PlaylistExclusionsBridgeFactory

+ (id<PlaylistExclusionsBridge>)sharedBridge {
  static PlaylistExclusionsBridgeImpl* instance = nil;
  static dispatch_once_t once;
  dispatch_once(&once, ^{
    instance = [[PlaylistExclusionsBridgeImpl alloc]
        initWithPlaylistExclusions:*playlist::PlaylistExclusions::
                                       GetInstance()];
  });
  return instance;
}

@end
