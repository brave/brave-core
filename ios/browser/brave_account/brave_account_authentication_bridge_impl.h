/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_BRAVE_ACCOUNT_BRAVE_ACCOUNT_AUTHENTICATION_BRIDGE_IMPL_H_
#define BRAVE_IOS_BROWSER_BRAVE_ACCOUNT_BRAVE_ACCOUNT_AUTHENTICATION_BRIDGE_IMPL_H_

#import <Foundation/Foundation.h>

#include "brave_account_authentication_bridge.h"

@protocol ProfileBridge;

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT NSString* const BraveAccountAuthenticationTokenPref;
OBJC_EXPORT NSString* const BraveAccountVerificationTokenPref;

OBJC_EXPORT
@interface BraveAccountAuthenticationBridgeImpl
    : NSObject <BraveAccountAuthenticationBridge>
- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithProfile:(id<ProfileBridge>)profile
    NS_DESIGNATED_INITIALIZER;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_BRAVE_ACCOUNT_BRAVE_ACCOUNT_AUTHENTICATION_BRIDGE_IMPL_H_
