// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_WEB_VIEW_PUBLIC_CWV_FAVICON_STATUS_H_
#define BRAVE_IOS_WEB_VIEW_PUBLIC_CWV_FAVICON_STATUS_H_

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

// Collects the favicon related information for a NavigationItem.
OBJC_EXPORT
@interface CWVFaviconStatus : NSObject

// The URL of the favicon which was used to load it off the web.
@property(readonly, nullable) NSURL* url;

// The favicon bitmap for the page. It is fetched asynchronously after the
// favicon URL is set, so it is possible for `image` to be empty even when
// `url` is non-nil.
@property(readonly, nullable) UIImage* image;

- (instancetype)init NS_UNAVAILABLE;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_WEB_VIEW_PUBLIC_CWV_FAVICON_STATUS_H_
