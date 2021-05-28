/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "BraveCertificateTLSFeatureExtensionModel.h"
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

@implementation BraveCertificateTLSFeatureExtensionModel
- (void)parseExtension:(X509_EXTENSION*)extension {
  NSMutableArray* features = [[NSMutableArray alloc] init];
  
  #ifndef OPENSSL_IS_BORINGSSL
  //STACK_OF(ASN1_INTEGER) //Must Staple
  TLS_FEATURE* tls_feature = static_cast<TLS_FEATURE*>(X509V3_EXT_d2i(extension));
  if (tls_feature) {
    for (std::size_t i = 0; i < sk_ASN1_INTEGER_num(tls_feature); ++i) {
      ASN1_INTEGER* value = sk_ASN1_INTEGER_value(tls_feature, static_cast<int>(i));
      if (value) {
        long tls_extension_id = ASN1_INTEGER_get(value);

        //5 = status_request
        //17 = status_request_v2
        //unknown = UNKNOWN TLS FEATURE (Must Staple)
        [features addObject:@(tls_extension_id)];
      }
    }
    TLS_FEATURE_free(tls_feature);
  }
  #endif
  _features = features;
}
@end
