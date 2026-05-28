// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_PLAYLIST_PLAYLIST_EXCLUSIONS_FACTORY_H_
#define BRAVE_IOS_BROWSER_PLAYLIST_PLAYLIST_EXCLUSIONS_FACTORY_H_

#import <Foundation/Foundation.h>

@protocol PlaylistExclusions;

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
NS_SWIFT_NAME(PlaylistExclusionsFactory)
@interface PlaylistExclusionsFactory : NSObject

+ (id<PlaylistExclusions>)sharedPlaylistExclusions;

- (instancetype)init NS_UNAVAILABLE;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_PLAYLIST_PLAYLIST_EXCLUSIONS_FACTORY_H_
