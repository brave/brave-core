/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_WEB_VIEW_PUBLIC_CWV_WEB_VIEW_EXTRAS_H_
#define BRAVE_IOS_WEB_VIEW_PUBLIC_CWV_WEB_VIEW_EXTRAS_H_

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import <WebKit/WebKit.h>

#import "cwv_export.h"
#import "cwv_web_view.h"

typedef NSInteger CWVUserAgentType NS_TYPED_ENUM;
OBJC_EXPORT const CWVUserAgentType CWVUserAgentTypeNone;
OBJC_EXPORT const CWVUserAgentType CWVUserAgentTypeAutomatic;
OBJC_EXPORT const CWVUserAgentType CWVUserAgentTypeMobile;
OBJC_EXPORT const CWVUserAgentType CWVUserAgentTypeDesktop;

NS_ASSUME_NONNULL_BEGIN

CWV_EXPORT
@interface CWVWebView (Extras)

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

#endif  // BRAVE_IOS_WEB_VIEW_PUBLIC_CWV_WEB_VIEW_EXTRAS_H_
