// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_WEB_VIEW_PUBLIC_CHROME_WEB_VIEW_H_
#define BRAVE_IOS_WEB_VIEW_PUBLIC_CHROME_WEB_VIEW_H_

#import <Foundation/Foundation.h>
#import <WebKit/WebKit.h>

#import "cwv_export.h"               // NOLINT
#import "cwv_navigation_delegate.h"  // NOLINT
#import "cwv_web_view.h"             // NOLINT

typedef NSInteger CWVUserAgentType NS_TYPED_ENUM;
OBJC_EXPORT const CWVUserAgentType CWVUserAgentTypeNone;
OBJC_EXPORT const CWVUserAgentType CWVUserAgentTypeAutomatic;
OBJC_EXPORT const CWVUserAgentType CWVUserAgentTypeMobile;
OBJC_EXPORT const CWVUserAgentType CWVUserAgentTypeDesktop;

NS_ASSUME_NONNULL_BEGIN

CWV_EXPORT
@protocol BraveWebViewNavigationDelegate <CWVNavigationDelegate>
@optional
- (BOOL)webView:(CWVWebView*)webView
    shouldBlockUniversalLinksForRequest:(NSURLRequest*)request;
- (BOOL)webView:(CWVWebView*)webView
    shouldBlockJavaScriptForRequest:(NSURLRequest*)request;
- (void)webView:(CWVWebView*)webView
    didRequestHTTPAuthForProtectionSpace:(NSURLProtectionSpace*)protectionSpace
                      proposedCredential:(NSURLCredential*)proposedCredential
                       completionHandler:
                           (void (^)(NSString* _Nullable username,
                                     NSString* _Nullable password))handler;
@end

CWV_EXPORT
@interface BraveWebView : CWVWebView

@property(nonatomic, readonly, nullable) NSURL* visibleURL;

// This web view's navigation delegate.
@property(nonatomic, weak, nullable) id<BraveWebViewNavigationDelegate>
    navigationDelegate;

/// Determines if the data used to restore a CWVWebView is a WebState cache and
/// is valid.
///
/// Used for migration purposes, can be removed in the future
+ (BOOL)isRestoreDataValid:(NSData*)data;

- (void)updateScripts;
- (void)createPDF:(void (^)(NSData* _Nullable))completionHandler;
- (void)takeSnapshotWithRect:(CGRect)rect
           completionHandler:(void (^)(UIImage* _Nullable))completionHandler;
- (CWVUserAgentType)currentItemUserAgentType;
- (void)reloadWithUserAgentType:(CWVUserAgentType)userAgentType;
- (void)evaluateJavaScript:(NSString*)javaScriptString
              contentWorld:(WKContentWorld*)contentWorld
         completionHandler:
             (nullable void (^)(id _Nullable result,
                                NSError* _Nullable error))completion;

@property(readonly, nullable) WKWebView* underlyingWebView;
@property(readonly) WKWebViewConfiguration* WKConfiguration;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_WEB_VIEW_PUBLIC_CHROME_WEB_VIEW_H_
