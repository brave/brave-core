/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_NETWORKING_BRAVE_CERTIFICATE_MODEL_H_
#define BRAVE_IOS_BROWSER_API_NETWORKING_BRAVE_CERTIFICATE_MODEL_H_

#import <Foundation/Foundation.h>
#import <Security/Security.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_OPTIONS(NSUInteger, BravePublicKeyUsage) {
  BravePublicKeyUsage_Invalid = 1 << 0,
  BravePublicKeyUsage_Encrpyt = 1 << 1,
  BravePublicKeyUsage_Verify  = 1 << 2,
  BravePublicKeyUsage_Wrap    = 1 << 3,
  BravePublicKeyUsage_Derive  = 1 << 4,
  BravePublicKeyUsage_Any     = 1 << 5
};

typedef NS_ENUM(NSUInteger, BravePublicKeyType) {
  BravePublicKeyType_UNKNOWN,
  BravePublicKeyType_RSA,
  BravePublicKeyType_DSA,
  BravePublicKeyType_DH,
  BravePublicKeyType_EC
};

typedef NS_ENUM(NSUInteger, BraveFingerprintType) {
  BraveFingerprintType_SHA1,
  BraveFingerprintType_SHA256
};

OBJC_EXPORT
@interface BraveCertificateSubjectModel: NSObject
@property (nonatomic, strong, readonly) NSString* countryOrRegion;
@property (nonatomic, strong, readonly) NSString* stateOrProvince;
@property (nonatomic, strong, readonly) NSString* locality;
@property (nonatomic, strong, readonly) NSString* organization;
@property (nonatomic, strong, readonly) NSString* organizationalUnit;
@property (nonatomic, strong, readonly) NSString* commonName;
@property (nonatomic, strong, readonly) NSString* streetAddress;
@property (nonatomic, strong, readonly) NSString* domainComponent;
@property (nonatomic, strong, readonly) NSString* userId;
@end

OBJC_EXPORT
@interface BraveCertificateIssuerName: NSObject
@property (nonatomic, strong, readonly) NSString* countryOrRegion;
@property (nonatomic, strong, readonly) NSString* stateOrProvince;
@property (nonatomic, strong, readonly) NSString* locality;
@property (nonatomic, strong, readonly) NSString* organization;
@property (nonatomic, strong, readonly) NSString* organizationalUnit;
@property (nonatomic, strong, readonly) NSString* commonName;
@property (nonatomic, strong, readonly) NSString* streetAddress;
@property (nonatomic, strong, readonly) NSString* domainComponent;
@property (nonatomic, strong, readonly) NSString* userId;
@end

OBJC_EXPORT
@interface BraveCertificateSignature: NSObject
@property (nonatomic, strong, readonly) NSString* algorithm;
@property (nonatomic, strong, readonly) NSString* objectIdentifier;
@property (nonatomic, strong, readonly) NSString* signatureHexEncoded;
@property (nonatomic, strong, readonly) NSString* parameters;
@property (nonatomic, assign, readonly) NSUInteger bytesSize;
@end

OBJC_EXPORT
@interface BraveCertificatePublicKeyInfo: NSObject
@property (nonatomic, assign, readonly) BravePublicKeyType type;
@property (nonatomic, strong, readonly) NSString* algorithm;
@property (nonatomic, strong, readonly) NSString* objectIdentifier;
@property (nonatomic, strong, readonly) NSString* curveName;
@property (nonatomic, strong, readonly) NSString* nistCurveName;
@property (nonatomic, strong, readonly) NSString* parameters;
@property (nonatomic, strong, readonly) NSString* keyHexEncoded;
@property (nonatomic, assign, readonly) NSUInteger keyBytesSize;
@property (nonatomic, assign, readonly) NSUInteger exponent;
@property (nonatomic, assign, readonly) NSUInteger keySizeInBits;
@property (nonatomic, assign, readonly) BravePublicKeyUsage keyUsage;
@end

OBJC_EXPORT
@interface BraveCertificateFingerprint: NSObject
@property (nonatomic, assign, readonly) BraveFingerprintType type;
@property (nonatomic, strong, readonly) NSString* fingerprintHexEncoded;
@end

OBJC_EXPORT
@interface BraveCertificateModel: NSObject
@property (nonatomic, assign, readonly) bool isRootCertificate;
@property (nonatomic, assign, readonly) bool isCertificateAuthority;
@property (nonatomic, assign, readonly) bool isSelfSigned;
@property (nonatomic, assign, readonly) bool isSelfIssued;
@property (nonatomic, strong, readonly) BraveCertificateSubjectModel* subjectName;
@property (nonatomic, strong, readonly) BraveCertificateIssuerName* issuerName;
@property (nonatomic, strong, readonly) NSString* serialNumber;
@property (nonatomic, assign, readonly) NSUInteger version;
@property (nonatomic, strong, readonly) BraveCertificateSignature* signature;
@property (nonatomic, strong, readonly) NSDate* notValidBefore;
@property (nonatomic, strong, readonly) NSDate* notValidAfter;
@property (nonatomic, strong, readonly) BraveCertificatePublicKeyInfo* publicKeyInfo;
@property (nullable, nonatomic, strong, readonly) NSArray* extensions;
@property (nonatomic, strong, readonly) BraveCertificateFingerprint* sha1Fingerprint;
@property (nonatomic, strong, readonly) BraveCertificateFingerprint* sha256Fingerprint;

- (nullable instancetype)initWithData:(NSData *)data;
- (nullable instancetype)initWithFilePath:(NSString *)path;
- (nullable instancetype)initWithCertificate:(nonnull SecCertificateRef)certificate;
@end

NS_ASSUME_NONNULL_END

#endif // BRAVE_IOS_BROWSER_API_NETWORKING_BRAVE_CERTIFICATE_MODEL_H_
