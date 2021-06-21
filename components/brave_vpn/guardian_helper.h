/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_GUARDIAN_HELPER_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_GUARDIAN_HELPER_H_

#import <Foundation/Foundation.h>

typedef void (^StandardBlock)(BOOL success, NSString* _Nullable errorMessage);
typedef void (^ResponseBlock)(NSDictionary* _Nullable response,
                              NSString* _Nullable errorMessage,
                              BOOL success);

@class GRDRegion;

NS_ASSUME_NONNULL_BEGIN

@interface GRDVPNHelper : NSObject

+ (instancetype)sharedInstance;
+ (BOOL)activeConnectionPossible;
+ (BOOL)isPayingUser;
+ (void)clearVpnConfiguration;

- (void)configureFirstTimeUserWithRegion:(GRDRegion* _Nullable)region
                              completion:(StandardBlock)completion;
- (void)disconnectVPN;

- (void)proLoginWithEmail:(NSString* _Nonnull)email
                 password:(NSString* _Nonnull)password
               completion:(ResponseBlock)completion;
- (void)logoutCurrentProUser;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_GUARDIAN_HELPER_H_
