/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_OPENTABS_BRAVE_SENDTAB_API_H_
#define BRAVE_IOS_BROWSER_API_OPENTABS_BRAVE_SENDTAB_API_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSUInteger, TargetDeviceType) {
  TargetDeviceTypeUnset = 0,
  TargetDeviceTypePC,
  TargetDeviceTypeMobile,
  TargetDeviceTypeTablet,
};

@protocol SendTabToSelfModelStateObserver;
@protocol SendTabToSelfModelStateListener;

@class IOSSendTabTargetDevice;

NS_SWIFT_NAME(SendTabTargetDevice)
OBJC_EXPORT
@interface IOSSendTabTargetDevice : NSObject <NSCopying>

@property(nonatomic, strong) NSString* fullName;
@property(nonatomic, strong) NSString* shortName;
@property(nonatomic, strong) NSString* deviceName;
@property(nonatomic, strong) NSString* cacheId;
@property(nonatomic) TargetDeviceType deviceType;
@property(nonatomic, copy) NSDate* lastUpdatedTime;

/// Target Device Constructor used with SendTabAPI
/// @param fullName - Device full name
/// @param shortName - Device short name
/// @param deviceName - Device name
/// @param cacheId - Device cacheId
/// @param deviceType - Device type
/// @param lastUpdatedTime - Last updated timestamp
- (instancetype)initWithFullName:(NSString*)fullName
                       shortName:(NSString*)shortName
                      deviceName:(NSString*)deviceName
                         cacheId:(NSString*)cacheId
                      deviceType:(TargetDeviceType)deviceType
                 lastUpdatedTime:(NSDate*)lastUpdatedTime;
@end

NS_SWIFT_NAME(BraveSendTabAPI)
OBJC_EXPORT
@interface BraveSendTabAPI : NSObject

- (id<SendTabToSelfModelStateListener>)addObserver:
    (id<SendTabToSelfModelStateObserver>)observer;
- (void)removeObserver:(id<SendTabToSelfModelStateListener>)observer;

- (instancetype)init NS_UNAVAILABLE;

/// Fetch list of devices which will be used as receiver
- (NSArray<IOSSendTabTargetDevice*>*)getListOfSyncedDevices;

/// Send Tab URL & Title to a specific device
/// @param deviceID Id of the target device
/// @param tabTitle Title of the Tab being sent
/// @param activeURL URL of the Tab being sent
- (void)sendActiveTabToDevice:(NSString*)deviceID
                     tabTitle:(NSString*)tabTitle
                    activeURL:(NSURL*)activeURL;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_OPENTABS_BRAVE_SENDTAB_API_H_
