/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_CERTIFICATE_BRAVE_CERTIFICATE_H_
#define BRAVE_IOS_BROWSER_API_CERTIFICATE_BRAVE_CERTIFICATE_H_

#import <Foundation/Foundation.h>
#import <Security/Security.h>

NS_ASSUME_NONNULL_BEGIN

@class BraveCertificateRDNSequence;
@class BraveCertificateSignature;
@class BraveCertificatePublicKeyInfo;
@class BraveCertificateFingerprint;
// @class BraveCertificateExtensionModel;

OBJC_EXPORT
@interface BraveCertificateModel : NSObject
@property(nonatomic, readonly) bool isRootCertificate;
@property(nonatomic, readonly) bool isCertificateAuthority;
@property(nonatomic, readonly) bool isSelfSigned;
@property(nonatomic, readonly) bool isSelfIssued;
@property(nonatomic, readonly) BraveCertificateRDNSequence* subjectName;
@property(nonatomic, readonly) BraveCertificateRDNSequence* issuerName;
@property(nonatomic, readonly) NSString* serialNumber;
@property(nonatomic, readonly) NSUInteger version;
@property(nonatomic, readonly) BraveCertificateSignature* signature;
@property(nonatomic, readonly) NSDate* notValidBefore;
@property(nonatomic, readonly) NSDate* notValidAfter;
@property(nonatomic, readonly) BraveCertificatePublicKeyInfo* publicKeyInfo;
// @property(nonatomic, readonly)
// NSArray<BraveCertificateExtensionModel*>* extensions;
@property(nonatomic, readonly) BraveCertificateFingerprint* sha1Fingerprint;
@property(nonatomic, readonly) BraveCertificateFingerprint* sha256Fingerprint;

- (nullable instancetype)initWithCertificate:(SecCertificateRef)certificate;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_CERTIFICATE_BRAVE_CERTIFICATE_H_
