/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_OPENTABS_BRAVE_OPENTABS_API_H_
#define BRAVE_IOS_BROWSER_API_OPENTABS_BRAVE_OPENTABS_API_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef NSInteger SyncDeviceFormFactor NS_TYPED_ENUM;

OBJC_EXPORT SyncDeviceFormFactor const SyncDeviceFormFactorUnknown;
OBJC_EXPORT SyncDeviceFormFactor const SyncDeviceFormFactorDesktop;
OBJC_EXPORT SyncDeviceFormFactor const SyncDeviceFormFactorPhone;
OBJC_EXPORT SyncDeviceFormFactor const SyncDeviceFormFactorTablet;

@protocol OpenTabsSessionStateObserver;
@protocol OpenTabsSessionStateListener;

@class IOSOpenDistantTab;
@class IOSOpenDistantSession;

NS_SWIFT_NAME(OpenDistantTab)
OBJC_EXPORT
@interface IOSOpenDistantTab : NSObject <NSCopying>

@property(nonatomic, strong) NSURL* url;
@property(nonatomic, nullable, copy) NSString* title;
@property(nonatomic) NSInteger tabId;
@property(nonatomic, strong) NSString* sessionTag;

/// Open Tab Constructor used with OpenTabSessionAPI
/// @param url - Mandatory URL field for the open tab object
/// @param title - Title used for the URL
/// @param tabId - Uniquely identifies this tab in its distant session
/// @param sessionTag - Uniquely identifies the distant session this tab belongs
/// to
- (instancetype)initWithURL:(NSURL*)url
                      title:(nullable NSString*)title
                      tabId:(NSInteger)tabId
                 sessionTag:(NSString*)sessionTag;
@end

NS_SWIFT_NAME(OpenDistantSession)
OBJC_EXPORT
@interface IOSOpenDistantSession : NSObject <NSCopying>

@property(nonatomic, nullable, copy) NSString* name;
@property(nonatomic, copy) NSString* sessionTag;
@property(nonatomic, nullable, copy) NSDate* modifiedTime;
@property(nonatomic) SyncDeviceFormFactor deviceFormFactor;
@property(nonatomic, strong) NSArray<IOSOpenDistantTab*>* tabs;

/// Open Tab Constructor used with OpenTabSessionAPI
/// @param name - This is the name of the device for the distant session
/// @param sessionTag - Uniquely identifies the distant session
/// @param modifiedTime - The time last distant session is modified
/// @param deviceFormFactor - The type of synced device for the distant session
/// @param tabs - The open tabs synced with the particular session
- (instancetype)initWithName:(nullable NSString*)name
                  sessionTag:(NSString*)sessionTag
                 dateCreated:(nullable NSDate*)modifiedTime
            deviceFormFactor:(SyncDeviceFormFactor)deviceFormFactor;
@end

NS_SWIFT_NAME(BraveOpenTabsAPI)
OBJC_EXPORT
@interface BraveOpenTabsAPI : NSObject

- (id<OpenTabsSessionStateListener>)addObserver:
    (id<OpenTabsSessionStateObserver>)observer;
- (void)removeObserver:(id<OpenTabsSessionStateListener>)observer;

- (instancetype)init NS_UNAVAILABLE;

/// Fetch function that will return all synced sessions with tab information
- (NSArray<IOSOpenDistantSession*>*)getSyncedSessions;

/// Delete function that will be used for "Hide for now" action
/// @param sessionTag - This is the session tag belongs to the open tabs
/// session to be deleted
- (void)deleteSyncedSession:(NSString*)sessionTag;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_OPENTABS_BRAVE_OPENTABS_API_H_
