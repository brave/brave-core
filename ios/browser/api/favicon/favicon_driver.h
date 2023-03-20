/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_FAVICON_FAVICON_DRIVER_H_
#define BRAVE_IOS_BROWSER_API_FAVICON_FAVICON_DRIVER_H_

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import <Webkit/Webkit.h>

@class WebState;

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
NS_SWIFT_NAME(FaviconDriver)
@interface FaviconDriver : NSObject
- (instancetype)init NS_UNAVAILABLE;

- (instancetype)initWithWebState:(WebState*)webState NS_DESIGNATED_INITIALIZER;

- (void)setMaximumFaviconImageSize:(CGSize)maxImageSize;
- (void)webView:(WKWebView*)webView
       scriptMessage:(WKScriptMessage*)scriptMessage
    onFaviconUpdated:(void (^)(NSURL* _Nullable, UIImage* _Nullable))callback;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_FAVICON_FAVICON_DRIVER_H_
