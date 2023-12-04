// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_API_WEB_UI_CHROME_WEBVIEW_H_
#define BRAVE_IOS_BROWSER_API_WEB_UI_CHROME_WEBVIEW_H_

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import <WebKit/WebKit.h>

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
@interface ChromeWebViewController : UIViewController
@property(nonatomic, readonly) WKWebView* webView;
@property(nonatomic, readonly) bool isOffTheRecord;

- (instancetype)initWithPrivateBrowsing:(bool)isPrivateBrowsing;
- (void)loadURL:(NSString*)urlString;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_WEB_UI_CHROME_WEBVIEW_H_
