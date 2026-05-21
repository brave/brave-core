// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/playlist/core/browser/playlist_exclusion.h"

#include "base/memory/raw_ptr.h"
#include "brave/ios/browser/playlist/playlist_exclusion_ios+private.h"
#include "net/base/apple/url_conversions.h"
#include "url/gurl.h"

@implementation PlaylistExclusionsAPIImpl {
  raw_ptr<playlist::PlaylistExclusions> _playlistExclusions;  // NOT OWNED
}

- (instancetype)initWithPlaylistExclusions:
    (playlist::PlaylistExclusions*)playlistExclusions {
  if ((self = [super init])) {
    _playlistExclusions = playlistExclusions;
  }
  return self;
}

- (bool)canResolvePageSrcLater:(NSURL*)url {
  GURL gurl = net::GURLWithNSURL(url);
  if (!gurl.is_valid()) {
    return true;
  }
  return _playlistExclusions->CanResolvePageSrcLater(gurl);
}

@end

@implementation BravePlaylistExclusions

+ (id<PlaylistExclusionsAPI>)sharedInstance {
  static PlaylistExclusionsAPIImpl* instance = nil;
  static dispatch_once_t once;
  dispatch_once(&once, ^{
    instance = [[PlaylistExclusionsAPIImpl alloc]
        initWithPlaylistExclusions:playlist::PlaylistExclusions::GetInstance()];
  });
  return instance;
}

@end
