// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_BRAVE_ORIGIN_BRAVE_ORIGIN_SERVICE_BRIDGE_H_
#define BRAVE_IOS_BROWSER_BRAVE_ORIGIN_BRAVE_ORIGIN_SERVICE_BRIDGE_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef NSString* BraveOriginPolicyKey NS_TYPED_EXTENSIBLE_ENUM;
OBJC_EXPORT BraveOriginPolicyKey const BraveOriginPolicyKeyWalletDisabled;
OBJC_EXPORT BraveOriginPolicyKey const BraveOriginPolicyKeyAIChatEnabled;
OBJC_EXPORT BraveOriginPolicyKey const BraveOriginPolicyKeyRewardsDisabled;
OBJC_EXPORT BraveOriginPolicyKey const BraveOriginPolicyKeyTalkDisabled;
OBJC_EXPORT BraveOriginPolicyKey const BraveOriginPolicyKeyNewsDisabled;
OBJC_EXPORT BraveOriginPolicyKey const BraveOriginPolicyKeyVPNDisabled;
OBJC_EXPORT BraveOriginPolicyKey const BraveOriginPolicyKeyP3AEnabled;
OBJC_EXPORT BraveOriginPolicyKey const BraveOriginPolicyKeyStatsPingEnabled;

NS_SWIFT_NAME(BraveOriginService)
@protocol BraveOriginServiceBridge

// Check if a policy is controlled by BraveOrigin
- (BOOL)isPolicyControlledByBraveOrigin:(BraveOriginPolicyKey)policyKey;

// Update the BraveOrigin policy value
- (BOOL)setPolicyValue:(BraveOriginPolicyKey)policyKey value:(BOOL)value;

// Get the current value of a BraveOrigin policy
// Returns nil if the policy value is not set
- (nullable NSNumber*)getPolicyValue:(BraveOriginPolicyKey)policyKey;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_BRAVE_ORIGIN_BRAVE_ORIGIN_SERVICE_BRIDGE_H_
