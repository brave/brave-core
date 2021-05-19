/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */


#ifndef BRAVE_IOS_BROWSER_API_NETWORKING_BRAVE_CERTIFICATE_MODEL_H_
#define BRAVE_IOS_BROWSER_API_NETWORKING_BRAVE_CERTIFICATE_MODEL_H_

#import <Foundation/Foundation.h>
#import <Security/Security.h>

NS_ASSUME_NONNULL_BEGIN

@class BraveCertificateSubjectModel;
@class BraveCertificateIssuerName;
@class BraveCertificateSignature;
@class BraveCertificatePublicKeyInfo;
@class BraveCertificateFingerprint;
@class BraveCertificateExtensionModel;

OBJC_EXPORT
@interface BraveCertificateModel: NSObject
@property(nonatomic, assign, readonly) bool isRootCertificate;
@property(nonatomic, assign, readonly) bool isCertificateAuthority;
@property(nonatomic, assign, readonly) bool isSelfSigned;
@property(nonatomic, assign, readonly) bool isSelfIssued;
@property(nonatomic, strong, readonly) BraveCertificateSubjectModel* subjectName;
@property(nonatomic, strong, readonly) BraveCertificateIssuerName* issuerName;
@property(nonatomic, strong, readonly) NSString* serialNumber;
@property(nonatomic, assign, readonly) NSUInteger version;
@property(nonatomic, strong, readonly) BraveCertificateSignature* signature;
@property(nonatomic, strong, readonly) NSDate* notValidBefore;
@property(nonatomic, strong, readonly) NSDate* notValidAfter;
@property(nonatomic, strong, readonly) BraveCertificatePublicKeyInfo* publicKeyInfo;
@property(nonatomic, strong, readonly) NSArray<BraveCertificateExtensionModel*>* extensions;
@property(nonatomic, strong, readonly) BraveCertificateFingerprint* sha1Fingerprint;
@property(nonatomic, strong, readonly) BraveCertificateFingerprint* sha256Fingerprint;

- (nullable instancetype)initWithData:(NSData *)data;
- (nullable instancetype)initWithFilePath:(NSString *)path;
- (nullable instancetype)initWithCertificate:(nonnull SecCertificateRef)certificate;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_NETWORKING_BRAVE_CERTIFICATE_MODEL_H_
