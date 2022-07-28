/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_FAVICON_FAVICON_DRIVER_H_
#define BRAVE_IOS_BROWSER_API_FAVICON_FAVICON_DRIVER_H_

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import <Webkit/Webkit.h>

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
NS_SWIFT_NAME(FaviconLoader.Driver)
@interface BraveFaviconDriver : NSObject
- (instancetype)initWithPrivateBrowsingMode:(bool)privateMode;

- (void)setMaximumFaviconImageSize:(NSUInteger)maxImageSize;
- (void)webView:(WKWebView*)webView
    onFaviconURLsUpdated:(WKScriptMessage*)scriptMessage;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_FAVICON_FAVICON_DRIVER_H_
