/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_OPENTABS_BRAVE_OPENTABS_API_H_
#define BRAVE_IOS_BROWSER_API_OPENTABS_BRAVE_OPENTABS_API_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

// @protocol OpenTabsSessionServiceObserver;
// @protocol OpenTabsSessionServiceListener;

@class IOSOpenTabNode;

NS_SWIFT_NAME(OpenTabNode)
OBJC_EXPORT
@interface IOSOpenTabNode : NSObject <NSCopying>

@property(nonatomic, strong) NSURL* url;
@property(nonatomic, nullable, copy) NSString* title;
@property(nonatomic) NSUInteger tabId;
@property(nonatomic, nullable, copy) NSString* sessionTag;

/// Open Tab Constructor used with OpenTabSessionAPI
/// @param url - Mandatory URL field for the open tab object
/// @param title - Title used for the URL
/// @param tabId - Uniquely identifies this tab in its distant session
/// @param sessionTag - Uniquely identifies the distant session this tab belongs to
- (instancetype)initWithURL:(NSURL*)url
                      title:(nullable NSString*)title
                      tabId:(NSUInteger)tabId
                 sessionTag:(nullable NSString*)sessionTag;
@end

NS_SWIFT_NAME(BraveOpenTabsAPI)
OBJC_EXPORT
@interface BraveOpenTabsAPI : NSObject

// - (id<OpenTabsSessionServiceListener>)addObserver:(id<OpenTabsSessionServiceObserver>)observer;
// - (void)removeObserver:(id<OpenTabsSessionServiceListener>)observer;

- (instancetype)init NS_UNAVAILABLE;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_OPENTABS_BRAVE_OPENTABS_API_H_