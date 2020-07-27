// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#import <Foundation/Foundation.h>

#import "GRDVPNHelper.h"

NS_ASSUME_NONNULL_BEGIN

@interface GRDSubscriberCredential : NSObject

@property (nonatomic, strong) NSString *subscriberCredential;
@property (nonatomic, strong) NSString *subscriptionType;
@property (nonatomic, strong) NSString *subscriptionTypePretty;
@property (nonatomic) NSInteger tokenExpirationDate;
@property (nonatomic) NSInteger subscriptionExpirationDate;

@property (nonatomic) BOOL tokenExpired;


- (instancetype)initWithSubscriberCredential:(NSString *)subscriberCredential;
- (void)processSubscriberCredentialInformation;

@end

NS_ASSUME_NONNULL_END
