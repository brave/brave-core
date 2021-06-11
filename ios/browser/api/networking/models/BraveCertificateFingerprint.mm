/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#if defined(BRAVE_CORE) // Compiling in Brave-Core
  #import "brave/ios/browser/api/networking/models/BraveCertificateFingerprint.h"
  #include "brave/ios/browser/api/networking/utils/brave_certificate_utils.h"
  #include "brave/ios/browser/api/networking/utils/brave_certificate_ios_utils.h"
#else
  #import "BraveCertificateFingerprint.h"
  #include "brave_certificate_utils.h"
  #include "brave_certificate_ios_utils.h"
#endif

@implementation BraveCertificateFingerprint
- (instancetype)initWithCertificate:(X509*)certificate withType:(BraveFingerprintType)type {
  if ((self = [super init])) {
    //OpenSSL_add_all_digests()
    _type = type;
    
    switch (type) {
      case BraveFingerprintType_SHA1: {
        _fingerprintHexEncoded =
            brave::string_to_ns(
                              x509_utils::fingerprint_with_nid(certificate,
                                                               NID_sha1));
      }
        break;
      case BraveFingerprintType_SHA256:{
        _fingerprintHexEncoded =
            brave::string_to_ns(
                              x509_utils::fingerprint_with_nid(certificate,
                                                               NID_sha256));
      }
        break;
    }
  }
  return self;
}
@end
