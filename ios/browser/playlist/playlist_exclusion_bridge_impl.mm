// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/playlist/playlist_exclusion_bridge_impl.h"

#include "base/memory/raw_ptr.h"
#include "brave/components/playlist/core/browser/playlist_exclusion.h"
#include "net/base/apple/url_conversions.h"
#include "url/gurl.h"

@implementation PlaylistExclusionsBridgeImpl {
  // `raw_ref` cannot be used here because ObjC ivars are zero-initialized
  // before `init` runs, but `raw_ref` cannot represent a null or default value.
  raw_ptr<playlist::PlaylistExclusions> _playlistExclusions;  // Not owned.
}

- (instancetype)initWithPlaylistExclusions:
    (playlist::PlaylistExclusions&)playlistExclusions {
  if ((self = [super init])) {
    _playlistExclusions = &playlistExclusions;
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
