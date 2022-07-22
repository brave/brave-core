/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_FAVICON_FAVICON_LOADER_H_
#define BRAVE_IOS_BROWSER_API_FAVICON_FAVICON_LOADER_H_

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

@class FaviconAttributes;

typedef NSInteger BraveFaviconLoaderSize
    NS_TYPED_ENUM NS_SWIFT_NAME(FaviconLoader.Sizes);
/// Minimal acceptable favicon size, in points.
OBJC_EXPORT BraveFaviconLoaderSize const BraveFaviconLoaderSizeMin;
/// Desired small favicon size, in points.
OBJC_EXPORT BraveFaviconLoaderSize const BraveFaviconLoaderSizeDesiredSmall;
/// Desired medium favicon size, in points.
OBJC_EXPORT BraveFaviconLoaderSize const BraveFaviconLoaderSizeDesiredMedium;

NS_ASSUME_NONNULL_BEGIN

// A class that manages asynchronously loading favicons or fallback attributes
// from LargeIconService and caching them, given a URL.
OBJC_EXPORT
NS_SWIFT_NAME(FaviconLoader)
@interface BraveFaviconLoader : NSObject

- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)getForPrivateMode:(bool)privateMode;

/// |sizeInPoints| - the desired size of the favIcon
/// |minSizeInPoints| - the minimum acceptable favIcon size
/// |completion| may be called more than once (once with a default image, and
/// one with the actual fav-icon if found).
- (void)faviconForPageURL:(NSURL*)url
              sizeInPoints:(BraveFaviconLoaderSize)sizeInPoints
           minSizeInPoints:(BraveFaviconLoaderSize)minSizeInPoints
    fallbackToGoogleServer:(bool)fallbackToGoogleServer
                completion:(void (^)(FaviconAttributes* attributes))completion;

/// |sizeInPoints| the desired size of the favIcon
/// |completion| may be called more than once (once with a default image, and
/// one with the actual fav-icon if found).
- (void)faviconForPageURLOrHost:(NSURL*)url
                   sizeInPoints:(BraveFaviconLoaderSize)sizeInPoints
                     completion:
                         (void (^)(FaviconAttributes* attributes))completion;

/// |sizeInPoints| the desired size of the favIcon
/// |completion| may be called more than once (once with a default image, and
/// one with the actual fav-icon if found).
- (void)faviconForIconURL:(NSURL*)url
             sizeInPoints:(BraveFaviconLoaderSize)sizeInPoints
          minSizeInPoints:(BraveFaviconLoaderSize)minSizeInPoints
               completion:(void (^)(FaviconAttributes* attributes))completion;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_FAVICON_FAVICON_LOADER_H_
