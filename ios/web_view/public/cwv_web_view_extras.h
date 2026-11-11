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

@class CWVBackForwardListItem;
@class CWVFaviconStatus;

NS_ASSUME_NONNULL_BEGIN

/// A handle to an in-flight download of the currently displayed page started
/// via `-[CWVWebView downloadCurrentPageToPath:completionHandler:]`.
///
/// The download is cancelled if this handle is deallocated before it
/// completes, so callers must retain it for the duration of the download.
CWV_EXPORT
@interface CWVWebViewDownload : NSObject

- (instancetype)init NS_UNAVAILABLE;

/// Cancels the download. The download's completion handler is invoked with an
/// `NSURLErrorCancelled` error if it has not already completed.
- (void)cancel;

@end

/// Adds additional functionality to CWVWebView that is not be supported out
/// of the box but can be implemented using the underlying WebState
CWV_EXPORT
@interface CWVWebView (Extras)

// Returns a unique identifier for this WebState that is stable across
// restart of the application
@property(readonly) int32_t uniqueSessionID;

// Web view's obscured insets.
@property(nonatomic) UIEdgeInsets obscuredInsets;

/// Sets the web view's min and max viewport insets
- (void)setMinimumViewportInset:(UIEdgeInsets)minInset
           maximumViewportInset:(UIEdgeInsets)maxInset;

/// The user agent type currently used on the page (e.g. mobile/desktop)
- (CWVUserAgentType)currentItemUserAgentType;

/// Reloads the page with a specific user agent type
- (void)reloadWithUserAgentType:(CWVUserAgentType)userAgentType;

/// Return the last committed navigation's original URL request
///
/// This is the same as WebKit's back/forward list current item `initialURL`
/// property.
@property(readonly, nullable)
    NSURL* originalRequestURLForLastCommitedNavigation;

/// The MIME type for the contents currently loaded in the web view
@property(readonly) NSString* contentsMIMEType;

/// The last time that the web view was active
@property(readonly) NSDate* lastActiveTime;

/// Returns a list of all non-redirected NavigationItems whose index precedes
/// or follows the current index.
@property(readonly) NSArray<CWVBackForwardListItem*>* backList;
@property(readonly) NSArray<CWVBackForwardListItem*>* forwardList;

/// Whether or not the keyboard is visible for this web views contents
@property(readonly, getter=isKeyboardVisible) BOOL keyboardVisible;

/// The current favicon status of the web state
@property(readonly, nullable) CWVFaviconStatus* faviconStatus;

/// Whether or not find in page is visible
@property(readonly, getter=isFindNavigatorVisible) BOOL findNavigatorVisible;

#pragma mark -

/// Creates a PDF of the current page
///
/// Equivalent of -[WKWebView
/// createPDFWithConfiguration:completionHandler:]
- (void)createFullPagePDF:(void (^)(NSData* _Nullable))completionHandler;

/// Downloads the last committed page to the file at `path` using the web
/// view's network session (cookies, auth, etc.).
///
/// `completionHandler` is invoked exactly once on the main thread with `nil`
/// on success or an error on failure or cancellation. Returns nil (after
/// invoking `completionHandler` with an error) if the download could not be
/// started.
- (nullable CWVWebViewDownload*)downloadCurrentPageToPath:(NSString*)path
                                        completionHandler:
                                            (void (^)(NSError* _Nullable error))
                                                completionHandler;

/// Returns a filename suitable for saving the last committed page to disk.
///
/// The name is derived from `contentDisposition` if provided, otherwise from
/// the last committed URL, and is given an extension matching the page's MIME
/// type when one is missing or unreliable (e.g. blob: URLs).
- (NSString*)suggestedFilenameForCurrentPageWithContentDisposition:
    (nullable NSString*)contentDisposition
    NS_SWIFT_NAME(suggestedFilenameForCurrentPage(contentDisposition:));

/// Whether or not you can create a snapshot using `takeSnapshotWithRect`
- (BOOL)canTakeSnapshot;

/// Creates an image from the current rendered page for a given CGRect
///
/// Equivalent of -[WKWebView takeSnapshotWithConfiguration:completionHandler:]
- (void)takeSnapshotWithRect:(CGRect)rect
           completionHandler:(void (^)(UIImage* _Nullable))completionHandler;

/// Evaluates JavaScript on the page on a given WebKit content world
///
/// Equivalent of -[WKWebView
/// evaluateJavaScript:contentWorld:completionHandler:]
- (void)evaluateJavaScript:(NSString*)javaScriptString
              contentWorld:(WKContentWorld*)contentWorld
         completionHandler:
             (nullable void (^)(id _Nullable result,
                                NSError* _Nullable error))completion;

#pragma mark -

/// Determines if the data used to restore a CWVWebView is a WebState cache and
/// is valid.
///
/// Used for migration purposes, can be removed in the future
+ (BOOL)isRestoreDataValid:(NSData*)data;

/// Resets & reinjects all JavaScript features
- (void)updateScripts;

/// The underlying WKWebView if one has been created already.
///
/// This is only available for `use_blink=false` builds and be used for WebKit
/// specific paths.
@property(readonly, nullable) WKWebView* internalWebView;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_WEB_VIEW_PUBLIC_CWV_WEB_VIEW_EXTRAS_H_
