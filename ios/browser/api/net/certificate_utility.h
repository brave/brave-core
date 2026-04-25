/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_NET_CERTIFICATE_UTILITY_H_
#define BRAVE_IOS_BROWSER_API_NET_CERTIFICATE_UTILITY_H_

#import <Foundation/Foundation.h>
#import <WebKit/WebKit.h>

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
@interface BraveCertificateUtility : NSObject
@property(nonatomic, readonly, class) NSArray<NSData*>* acceptableSPKIHashes;

+ (nullable NSString*)pemEncodeCertificate:(SecCertificateRef)certificate;
+ (nullable NSData*)hashCertificateSPKI:(SecCertificateRef)certificate;
+ (int)verifyTrust:(SecTrustRef)trust host:(NSString*)host port:(NSInteger)port;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_NET_CERTIFICATE_UTILITY_H_
