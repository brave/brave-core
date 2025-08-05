// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_API_PROFILE_PROFILE_BRIDGE_IMPL_H_
#define BRAVE_IOS_BROWSER_API_PROFILE_PROFILE_BRIDGE_IMPL_H_

#import <Foundation/Foundation.h>

#include "base/memory/raw_ptr.h"
#include "brave/ios/browser/api/profile/profile_bridge.h"

class ProfileIOS;

NS_ASSUME_NONNULL_BEGIN

/// An implementation that houses a Chrome ProfileIOS to implement the
/// required ProfileBridge methods
@interface ProfileBridgeImpl : NSObject <ProfileBridge>
@property(readonly) raw_ptr<ProfileIOS> profile;
- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithProfile:(raw_ptr<ProfileIOS>)profile
    NS_DESIGNATED_INITIALIZER;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_PROFILE_PROFILE_BRIDGE_IMPL_H_
