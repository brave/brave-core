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

@protocol ChromeWebViewControllerUIDelegate;
OBJC_EXPORT
@interface ChromeWebViewController : UIViewController
@property(nonatomic, readonly) WKWebView* webView;
@property(nonatomic, readonly) bool isOffTheRecord;
@property(nonatomic, weak) id<ChromeWebViewControllerUIDelegate> delegate;

- (instancetype)initWithPrivateBrowsing:(bool)isPrivateBrowsing;
- (void)loadURL:(NSString*)urlString;
@end

OBJC_EXPORT
@protocol ChromeWebViewControllerUIDelegate <NSObject>

/**
 * Called when the web page wants to open a new window.
 *
 * @param URL The url of the new window.
 * @param openerURL the URL of the page which requested a window to be open.
 * @param initiatedByUser If action was caused by the user.
 */
@optional
- (void)chromeWebViewController:
            (ChromeWebViewController*)chromeWebViewController
               openNewWindowFor:(const NSURL*)newWindowURL
                      openerURL:(const NSURL*)openerURL
                initiatedByUser:(BOOL)initiatedByUser;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_WEB_UI_CHROME_WEBVIEW_H_
