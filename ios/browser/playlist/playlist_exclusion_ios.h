// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_PLAYLIST_PLAYLIST_EXCLUSION_IOS_H_
#define BRAVE_IOS_BROWSER_PLAYLIST_PLAYLIST_EXCLUSION_IOS_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
@protocol PlaylistExclusionsAPI
@required

/// Returns whether `pageSrc` may be re-resolved later (e.g. via
/// LivePlaylistWebLoader). Mirrors
/// `PlaylistExclusions::CanResolvePageSrcLater`.
- (bool)canResolvePageSrcLater:(NSURL*)url;

@end

OBJC_EXPORT
@interface BravePlaylistExclusions : NSObject
+ (id<PlaylistExclusionsAPI>)sharedInstance;
- (instancetype)init NS_UNAVAILABLE;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_PLAYLIST_PLAYLIST_EXCLUSION_IOS_H_
