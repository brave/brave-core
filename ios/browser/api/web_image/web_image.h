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

- (void)downloadImage:(NSURL*)url
         maxImageSize:(NSUInteger)maxImageSize
       animationDelay:(CGFloat)animationDelay
           completion:(void (^)(UIImage* _Nullable image,
                                NSInteger httpResponseCode,
                                NSURL* url))completion;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_WEB_IMAGE_WEB_IMAGE_H_
