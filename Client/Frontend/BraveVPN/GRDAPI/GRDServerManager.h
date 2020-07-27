// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#import <Foundation/Foundation.h>

#import "GRDVPNHelper.h"
#import "GRDHousekeepingAPI.h"

NS_ASSUME_NONNULL_BEGIN

@interface GRDServerManager : NSObject

- (void)selectGuardianHostWithCompletion:(void (^)(NSString * _Nullable guardianHost, NSString * _Nullable errorMessage))completion;
- (void)getGuardianHostsWithCompletion:(void (^)(NSArray * _Nullable servers, NSString * _Nullable errorMessage))completion;

@end

NS_ASSUME_NONNULL_END
