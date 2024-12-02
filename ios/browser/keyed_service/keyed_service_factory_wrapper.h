/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_KEYED_SERVICE_KEYED_SERVICE_FACTORY_WRAPPER_H_
#define BRAVE_IOS_BROWSER_KEYED_SERVICE_KEYED_SERVICE_FACTORY_WRAPPER_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

/// A wrapper to obtain a Brave or Chromium keyed service based on the current
/// browser state.
///
/// Create a subclass of this class for each factory you want to expose to Swift
/// using the concrete return type and then implement the required
/// `serviceForProfile:` method.
///
/// This must be an Obj-C interface instead of a protocol due to limitations
/// of Obj-C's lightweight generics
@interface KeyedServiceFactoryWrapper<__covariant ResultType> : NSObject

/// Obtain the desired service based on whether or not you are
/// in private browsing mode.
///
/// Depending on the type of service you are requesting, you may receive
/// the same service regardless of private mode, or you may receive `nil` if
/// the service you requested does not support private browsing.
+ (nullable ResultType)getForPrivateMode:(bool)isPrivateBrowsing
    NS_SWIFT_NAME(get(privateMode:)) NS_REQUIRES_SUPER NS_SWIFT_UI_ACTOR;

- (instancetype)init NS_UNAVAILABLE;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_KEYED_SERVICE_KEYED_SERVICE_FACTORY_WRAPPER_H_
