// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_API_WEB_VIEW_BRAVE_WEB_FRAME_H_
#define BRAVE_IOS_BROWSER_API_WEB_VIEW_BRAVE_WEB_FRAME_H_

#import <Foundation/Foundation.h>

#import "cwv_export.h"  // NOLINT

@class URLOriginIOS;

NS_ASSUME_NONNULL_BEGIN

CWV_EXPORT
@interface BraveWebFrame : NSObject
/// The frame identifier which uniquely identifies this frame across the
/// application's lifetime.
@property(nonatomic, readonly) NSString* frameID;
/// Whether or not the receiver represents the main frame of the webpage.
@property(nonatomic, readonly, getter=isMainFrame) BOOL mainFrame;
/// The security origin associated with this frame.
@property(nonatomic, readonly) URLOriginIOS* securityOrigin;
/// The URL of the frame
@property(nonatomic, readonly, nullable) NSURL* url;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_WEB_VIEW_BRAVE_WEB_FRAME_H_
