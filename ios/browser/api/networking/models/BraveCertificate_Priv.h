/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BraveCertificateBasicModel_h
#define BraveCertificateBasicModel_h

#if defined(BRAVE_CORE) // Compiling in Brave-Core
  #import "brave/ios/browser/api/networking/common/brave_certificate_enums.h"
  #import "brave/ios/browser/api/networking/models/BraveCertificateSubjectModel.h"
  #import "brave/ios/browser/api/networking/models/BraveCertificateIssuerName.h"
  #import "brave/ios/browser/api/networking/models/BraveCertificateSignature.h"
  #import "brave/ios/browser/api/networking/models/BraveCertificatePublicKeyInfo.h"
  #import "brave/ios/browser/api/networking/models/BraveCertificateFingerprint.h"
#else
  #import "brave_certificate_enums.h"
  #import "BraveCertificateSubjectModel.h"
  #import "BraveCertificateIssuerName.h"
  #import "BraveCertificateSignature.h"
  #import "BraveCertificatePublicKeyInfo.h"
  #import "BraveCertificateFingerprint.h"
#endif

#if defined(BRAVE_CORE) // Compiling in Brave-Core
  #include "third_party/boringssl/src/include/openssl/x509.h"
  #include "third_party/boringssl/src/include/openssl/x509v3.h"
  #include "third_party/boringssl/src/include/openssl/asn1.h"
#else
  #include <openssl/x509.h>
  #include <openssl/x509v3.h>
  #include <openssl/asn1.h>
#endif

@interface BraveCertificateSubjectModel(Private)
- (instancetype)initWithCertificate:(X509*)certificate;
@end

@interface BraveCertificateIssuerName(Private)
- (instancetype)initWithCertificate:(X509*)certificate;
@end

@interface BraveCertificateSignature(Private)
- (instancetype)initWithCertificate:(X509*)certificate;
@end

@interface BraveCertificatePublicKeyInfo(Private)
- (instancetype)initWithCertificate:(X509*)certificate;
@end

@interface BraveCertificateFingerprint(Private)
- (instancetype)initWithCertificate:(X509*)certificate withType:(BraveFingerprintType)type;
@end

#endif /* BraveCertificateBasicModel_h */
