/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "BraveCertificatePrivateKeyUsagePeriodExtensionModel.h"
#if defined(BRAVE_CORE) // Compiling in Brave-Core
  #import "brave/ios/browser/api/networking/common/brave_certificate_enums.h"
  #include "brave/ios/browser/api/networking/utils/brave_certificate_ios_utils.h"
  #include "brave/ios/browser/api/networking/utils/brave_certificate_utils.h"
#else
  #import "brave_certificate_enums.h"
  #include "brave_certificate_ios_utils.h"
  #include "brave_certificate_utils.h"
#endif

#if defined(BRAVE_CORE) // Compiling in Brave-Core
  #include "third_party/boringssl/src/include/openssl/x509.h"
  #include "third_party/boringssl/src/include/openssl/x509v3.h"
#else
  #include <openssl/x509.h>
  #include <openssl/x509v3.h>
#endif

@implementation BraveCertificatePrivateKeyUsagePeriodExtensionModel
- (void)parseExtension:(X509_EXTENSION*)extension {
  #ifndef OPENSSL_IS_BORINGSSL
  PKEY_USAGE_PERIOD* key_usage_period = static_cast<PKEY_USAGE_PERIOD*>(X509V3_EXT_d2i(extension));
  if (key_usage_period) {
    // OpenSSL documentation:
    // The functions starting with ASN1_TIME will operate on either format.
    // So we can convert ASN1_GENERALIZED_TIME to timestamp using the same function.
    if (key_usage_period->notBefore) {
      _notBefore = brave::date_to_ns(x509_utils::date_from_ASN1TIME(key_usage_period->notBefore));
    }

    if (key_usage_period->notAfter) {
      _notAfter = brave::date_to_ns(x509_utils::date_from_ASN1TIME(key_usage_period->notAfter));
    }
    PKEY_USAGE_PERIOD_free(key_usage_period);
  }
  #endif  //  OPENSSL_IS_BORINGSSL
}
@end