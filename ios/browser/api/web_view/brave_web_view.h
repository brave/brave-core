// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_API_WEB_VIEW_BRAVE_WEB_VIEW_H_
#define BRAVE_IOS_BROWSER_API_WEB_VIEW_BRAVE_WEB_VIEW_H_

#import <Foundation/Foundation.h>
#import <WebKit/WebKit.h>

#import "cwv_export.h"               // NOLINT
#import "cwv_navigation_delegate.h"  // NOLINT
#import "cwv_web_view.h"             // NOLINT

NS_ASSUME_NONNULL_BEGIN

/// Additional navigation delegate methods that extend functionality of
/// CWVWebView
CWV_EXPORT
@protocol BraveWebViewNavigationDelegate <CWVNavigationDelegate>
@optional
/// Decides whether or not universal links should be blocked for a given request
- (BOOL)webView:(CWVWebView*)webView
    shouldBlockUniversalLinksForRequest:(NSURLRequest*)request;
/// Decides whether or not JavaScript should be blocked on the resulting page
- (BOOL)webView:(CWVWebView*)webView
    shouldBlockJavaScriptForRequest:(NSURLRequest*)request;
/// Notifies the delegate that basic authentication is required to access the
/// requested resource
- (void)webView:(CWVWebView*)webView
    didRequestHTTPAuthForProtectionSpace:(NSURLProtectionSpace*)protectionSpace
                      proposedCredential:(NSURLCredential*)proposedCredential
                       completionHandler:
                           (void (^)(NSString* _Nullable username,
                                     NSString* _Nullable password))handler;
@end

/// A CWVWebView with Chrome tab helpers attached and the ability to handle
/// some Brave specific features
CWV_EXPORT
@interface BraveWebView : CWVWebView

// This web view's navigation delegate.
@property(nonatomic, weak, nullable) id<BraveWebViewNavigationDelegate>
    navigationDelegate;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_WEB_VIEW_BRAVE_WEB_VIEW_H_
