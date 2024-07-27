/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_VERSION_INFO_VERSION_INFO_IOS_H_
#define BRAVE_IOS_BROWSER_API_VERSION_INFO_VERSION_INFO_IOS_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef NSInteger BraveCoreVersionInfoChannel
    NS_TYPED_ENUM NS_SWIFT_NAME(BraveCoreVersionInfo.Channel);

OBJC_EXPORT BraveCoreVersionInfoChannel const BraveCoreVersionInfoChannelStable;
OBJC_EXPORT BraveCoreVersionInfoChannel const BraveCoreVersionInfoChannelBeta;
OBJC_EXPORT BraveCoreVersionInfoChannel const
    BraveCoreVersionInfoChannelDevelopment;
OBJC_EXPORT BraveCoreVersionInfoChannel const
    BraveCoreVersionInfoChannelNightly;
OBJC_EXPORT BraveCoreVersionInfoChannel const
    BraveCoreVersionInfoChannelUnknown;

OBJC_EXPORT
@interface BraveCoreVersionInfo : NSObject
@property(class, readonly) NSString* braveCoreVersion;
@property(class, readonly) NSString* chromiumVersion;
@property(class, readonly) NSString* channelString;
@property(class, readonly) BraveCoreVersionInfoChannel channel;

- (instancetype)init NS_UNAVAILABLE;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_VERSION_INFO_VERSION_INFO_IOS_H_
