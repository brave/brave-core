// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_API_BRAVE_DOMAINS_SERVICE_DOMAINS_H_
#define BRAVE_IOS_BROWSER_API_BRAVE_DOMAINS_SERVICE_DOMAINS_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef NSInteger BraveServicesEnvironmentIOS
    NS_TYPED_ENUM NS_SWIFT_NAME(BraveDomains.ServicesEnvironment);

OBJC_EXPORT BraveServicesEnvironmentIOS const
    BraveServicesEnvironmentIOSDevelopment;
OBJC_EXPORT BraveServicesEnvironmentIOS const
    BraveServicesEnvironmentIOSStaging;
OBJC_EXPORT BraveServicesEnvironmentIOS const
    BraveServicesEnvironmentIOSProduction;

OBJC_EXPORT
@interface BraveDomains : NSObject
@property(class, nonatomic, readonly) BraveServicesEnvironmentIOS environment;

+ (BraveServicesEnvironmentIOS)environmentWithPrefix:(NSString* _Nonnull)prefix
    NS_SWIFT_NAME(enviroment(prefix:));
+ (NSString*)serviceDomainWithPrefix:(NSString*)prefix
    NS_SWIFT_NAME(serviceDomain(prefix:));
+ (NSString*)serviceDomainWithPrefix:(NSString*)prefix
                         environment:(BraveServicesEnvironmentIOS)environment
    NS_SWIFT_NAME(serviceDomain(prefix:environment:));

- (instancetype)init NS_UNAVAILABLE;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_BRAVE_DOMAINS_SERVICE_DOMAINS_H_
