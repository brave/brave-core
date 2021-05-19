/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
@interface BraveCertificateIssuerName: NSObject
@property(nonatomic, strong, readonly) NSString* countryOrRegion;
@property(nonatomic, strong, readonly) NSString* stateOrProvince;
@property(nonatomic, strong, readonly) NSString* locality;
@property(nonatomic, strong, readonly) NSString* organization;
@property(nonatomic, strong, readonly) NSString* organizationalUnit;
@property(nonatomic, strong, readonly) NSString* commonName;
@property(nonatomic, strong, readonly) NSString* streetAddress;
@property(nonatomic, strong, readonly) NSString* domainComponent;
@property(nonatomic, strong, readonly) NSString* userId;
@end

NS_ASSUME_NONNULL_END
