/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_FAVICON_FAVICON_LOADER_H_
#define BRAVE_IOS_BROWSER_API_FAVICON_FAVICON_LOADER_H_

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

@class FaviconAttributes;

typedef NSInteger FaviconLoaderSize
    NS_TYPED_ENUM NS_SWIFT_NAME(FaviconLoader.Sizes);
/// Smallest acceptable favicon size, in points.
OBJC_EXPORT FaviconLoaderSize const FaviconLoaderSizeDesiredSmallest;
/// Desired small favicon size, in points.
OBJC_EXPORT FaviconLoaderSize const FaviconLoaderSizeDesiredSmall;
/// Desired medium favicon size, in points.
OBJC_EXPORT FaviconLoaderSize const FaviconLoaderSizeDesiredMedium;
/// Desired large favicon size, in points.
OBJC_EXPORT FaviconLoaderSize const FaviconLoaderSizeDesiredLarge;
/// Desired larger favicon size, in points.
OBJC_EXPORT FaviconLoaderSize const FaviconLoaderSizeDesiredLarger;
/// Desired largest favicon size, in points.
OBJC_EXPORT FaviconLoaderSize const FaviconLoaderSizeDesiredLargest;

NS_ASSUME_NONNULL_BEGIN

// A class that manages asynchronously loading favicons or fallback attributes
// from LargeIconService and caching them, given a URL.
OBJC_EXPORT
NS_SWIFT_NAME(FaviconLoader)
@interface FaviconLoader : NSObject

- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)getForPrivateMode:(bool)privateMode;

/// |url - the page url
/// |sizeInPoints| - the desired size of the favIcon
/// |minSizeInPoints| - the minimum acceptable favIcon size
/// |completion| may be called more than once (once with a default image, and
/// one with the actual fav-icon if found).
- (void)faviconForPageURLOrHost:(NSURL*)url
                   sizeInPoints:(FaviconLoaderSize)sizeInPoints
                minSizeInPoints:(FaviconLoaderSize)minSizeInPoints
                     completion:
                         (void (^)(FaviconLoader* loader,
                                   FaviconAttributes* attributes))completion;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_FAVICON_FAVICON_LOADER_H_
