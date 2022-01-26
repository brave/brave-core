/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_CERTIFICATE_MODELS_BRAVE_CERTIFICATE_RDNS_SEQUENCE_H_
#define BRAVE_IOS_BROWSER_API_CERTIFICATE_MODELS_BRAVE_CERTIFICATE_RDNS_SEQUENCE_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
@interface BraveCertificateRDNSequence : NSObject
@property(nonatomic, readonly) NSString* countryOrRegion;
@property(nonatomic, readonly) NSString* stateOrProvince;
@property(nonatomic, readonly) NSString* locality;
@property(nonatomic, readonly) NSArray<NSString*>* organization;
@property(nonatomic, readonly) NSArray<NSString*>* organizationalUnit;
@property(nonatomic, readonly) NSString* commonName;
@property(nonatomic, readonly) NSArray<NSString*>* streetAddress;
@property(nonatomic, readonly) NSArray<NSString*>* domainComponent;
@property(nonatomic, readonly) NSString* userId;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_CERTIFICATE_MODELS_BRAVE_CERTIFICATE_RDNS_SEQUENCE_H_
