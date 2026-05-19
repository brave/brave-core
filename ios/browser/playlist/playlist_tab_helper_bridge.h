// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_PLAYLIST_PLAYLIST_TAB_HELPER_BRIDGE_H_
#define BRAVE_IOS_BROWSER_PLAYLIST_PLAYLIST_TAB_HELPER_BRIDGE_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@protocol PlaylistTabHelperBridge
@required

// Called when media is detected on the page by PlaylistJavaScriptFeature.
// `info` mirrors the `PlaylistItem` payload produced by playlist_utils.ts and
// can be decoded into a `PlaylistInfo`.
- (void)onMediaDetected:(NSDictionary<NSString*, id>*)info;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_PLAYLIST_PLAYLIST_TAB_HELPER_BRIDGE_H_
