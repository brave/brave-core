/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_WEB_IMAGE_WEB_IMAGE_H_
#define BRAVE_IOS_BROWSER_API_WEB_IMAGE_WEB_IMAGE_H_

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
@interface WebImageDownloader : NSObject
- (instancetype)init NS_UNAVAILABLE;

/// url - The URL to download the image from
- (void)downloadImage:(NSURL*)url
           completion:(void (^)(UIImage* _Nullable image,
                                NSInteger httpResponseCode,
                                NSURL* url))completion;

+ (nullable UIImage*)imageFromData:(NSData*)data;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_WEB_IMAGE_WEB_IMAGE_H_
